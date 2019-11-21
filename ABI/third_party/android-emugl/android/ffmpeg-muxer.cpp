// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

//
// Copyright (c) 2003 Fabrice Bellard
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "android/ffmpeg-muxer.h"

#include "android/base/synchronization/Lock.h"
#include "android/base/system/System.h"
#include "android/emulation/AudioCaptureEngine.h"
#include "android/ffmpeg-audio-capture.h"
#include "android/utils/debug.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavutil/avassert.h"
#include "libavutil/channel_layout.h"
#include "libavutil/mathematics.h"
#include "libavutil/opt.h"
#include "libavutil/timestamp.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
}

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#define STREAM_PIX_FMT AV_PIX_FMT_YUV420P /* default pix_fmt */

#define SCALE_FLAGS SWS_BICUBIC

using namespace android::emulation;

// a wrapper around a single output AVStream
struct VideoOutputStream {
    AVStream* st;

    // video width and height
    int width;
    int height;

    int bit_rate;
    int fps;

    // pts of the next frame that will be generated
    int64_t next_pts;

    AVFrame* frame;
    AVFrame* tmp_frame;

    struct SwsContext* sws_ctx;
};

typedef struct AudioOutputStream {
    AVStream* st;

    int bit_rate;
    int sample_rate;

    // pts of the next frame that will be generated
    int64_t next_pts;

    int samples_count;

    AVFrame* frame;
    AVFrame* tmp_frame;

    uint8_t* audio_leftover;
    int audio_leftover_len;

    struct SwrContext* swr_ctx;
} AudioOutputStream;

typedef struct ffmpeg_recorder {
    char* path;
    FfmpegAudioCapturer* audio_capturer;
    AVFormatContext* oc;
    VideoOutputStream video_st;
    AudioOutputStream audio_st;
    // A single lock to protect writing audio and video frames to the video file
    mutable android::base::Lock out_pkt_lock;
    bool have_video;
    bool have_audio;
    bool started;
    uint64_t start_time;
} ffmpeg_recorder;

// FFMpeg defines a set of macros for the same purpose, but those don't
// compile in C++ mode.
// Let's define regular C++ functions for it.
static std::string avTs2Str(int64_t ts) {
    char res[AV_TS_MAX_STRING_SIZE] = {};
    return av_ts_make_string(res, ts);
}

static std::string avTs2TimeStr(int64_t ts, AVRational* tb) {
    char res[AV_TS_MAX_STRING_SIZE] = {};
    return av_ts_make_time_string(res, ts, tb);
}

static std::string avErr2Str(int errnum) {
    char res[AV_ERROR_MAX_STRING_SIZE] = {};
    return av_make_error_string(res, AV_ERROR_MAX_STRING_SIZE, errnum);
}

static void log_packet(const AVFormatContext* fmt_ctx, const AVPacket* pkt) {
    AVRational* time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

    VERBOSE_PRINT(capture,
                  "pts:%s pts_time:%s dts:%s dts_time:%s duration:%s "
                  "duration_time:%s "
                  "stream_index:%d\n",
                  avTs2Str(pkt->pts).c_str(),
                  avTs2TimeStr(pkt->pts, time_base).c_str(),
                  avTs2Str(pkt->dts).c_str(),
                  avTs2TimeStr(pkt->dts, time_base).c_str(),
                  avTs2Str(pkt->duration).c_str(),
                  avTs2TimeStr(pkt->duration, time_base).c_str(),
                  pkt->stream_index);
}

static int write_frame(ffmpeg_recorder* recorder,
                       AVFormatContext* fmt_ctx,
                       const AVRational* time_base,
                       AVStream* st,
                       AVPacket* pkt) {
    // rescale output packet timestamp values from codec to stream timebase
    av_packet_rescale_ts(pkt, *time_base, st->time_base);
    pkt->stream_index = st->index;

    // Write the compressed frame to the media file.
    log_packet(fmt_ctx, pkt);

    android::base::AutoLock lock(recorder->out_pkt_lock);
    int rc = av_interleaved_write_frame(fmt_ctx, pkt);

    return rc;
}

// audio output
static AVFrame* alloc_audio_frame(enum AVSampleFormat sample_fmt,
                                  uint64_t channel_layout,
                                  int sample_rate,
                                  int nb_samples) {
    AVFrame* frame = av_frame_alloc();
    int ret;

    if (!frame) {
        derror("Error allocating an audio frame\n");
        return NULL;
    }

    frame->format = sample_fmt;
    frame->channel_layout = channel_layout;
    frame->sample_rate = sample_rate;
    frame->nb_samples = nb_samples;

    if (nb_samples) {
        ret = av_frame_get_buffer(frame, 0);
        if (ret < 0) {
            derror("Error allocating an audio buffer\n");
            return NULL;
        }
    }

    return frame;
}

static int open_audio(AVFormatContext* oc,
                      AVCodec* codec,
                      AudioOutputStream* ost,
                      AVDictionary* opt_arg) {
    AVCodecContext* c;
    int nb_samples;
    int ret;
    AVDictionary* opt = NULL;

    c = ost->st->codec;

    // open it
    av_dict_copy(&opt, opt_arg, 0);
    ret = avcodec_open2(c, codec, &opt);
    av_dict_free(&opt);
    if (ret < 0) {
        derror("Could not open audio codec: %s\n", avErr2Str(ret).c_str());
        return -1;
    }

    if (c->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
        nb_samples = 10000;
    else
        nb_samples = c->frame_size;

    ost->frame = alloc_audio_frame(c->sample_fmt, c->channel_layout,
                                   c->sample_rate, nb_samples);
    ost->tmp_frame = alloc_audio_frame(AV_SAMPLE_FMT_S16, c->channel_layout,
                                       c->sample_rate, nb_samples);

    // create resampler context
    ost->swr_ctx = swr_alloc();
    if (!ost->swr_ctx) {
        derror("Could not allocate resampler context\n");
        return -1;
    }

    // set options
    av_opt_set_int(ost->swr_ctx, "in_channel_count", c->channels, 0);
    av_opt_set_int(ost->swr_ctx, "in_sample_rate", c->sample_rate, 0);
    av_opt_set_sample_fmt(ost->swr_ctx, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0);
    av_opt_set_int(ost->swr_ctx, "out_channel_count", c->channels, 0);
    av_opt_set_int(ost->swr_ctx, "out_sample_rate", c->sample_rate, 0);
    av_opt_set_sample_fmt(ost->swr_ctx, "out_sample_fmt", c->sample_fmt, 0);

    // initialize the resampling context
    if ((ret = swr_init(ost->swr_ctx)) < 0) {
        derror("Failed to initialize the resampling context\n");
        return -1;
    }

    return 0;
}

//
// encode one audio frame and send it to the recorder
// return 1 when encoding is finished, 0 otherwise
//
static int write_audio_frame(ffmpeg_recorder* recorder,
                             AVFormatContext* oc,
                             AudioOutputStream* ost,
                             AVFrame* frame) {
    AVCodecContext* c;
    AVPacket pkt = {0};  // data and size must be 0;
    int ret;
    int got_packet;
    int dst_nb_samples;

    av_init_packet(&pkt);
    c = ost->st->codec;

    if (frame) {
        // convert samples from native format to destination codec format, using
        // the resampler compute destination number of samples
        dst_nb_samples = av_rescale_rnd(
                swr_get_delay(ost->swr_ctx, c->sample_rate) + frame->nb_samples,
                c->sample_rate, c->sample_rate, AV_ROUND_UP);
        av_assert0(dst_nb_samples == frame->nb_samples);

        // when we pass a frame to the encoder, it may keep a reference to it
        // internally;
        // make sure we do not overwrite it here
        //
        ret = av_frame_make_writable(ost->frame);
        if (ret < 0)
            return ret;

        // convert to destination format
        ret = swr_convert(ost->swr_ctx, ost->frame->data, dst_nb_samples,
                          (const uint8_t**)frame->data, frame->nb_samples);
        if (ret < 0) {
            derror("Error while converting\n");
            return ret;
        }
        frame = ost->frame;

        frame->pts =
                av_rescale_q(ost->samples_count,
                             (AVRational){1, c->sample_rate}, c->time_base);
        ost->samples_count += dst_nb_samples;
    }

    ret = avcodec_encode_audio2(c, &pkt, frame, &got_packet);
    if (ret < 0) {
        derror("Error encoding audio frame: %s\n", avErr2Str(ret).c_str());
        return ret;
    }

    if (got_packet) {
        ret = write_frame(recorder, oc, &c->time_base, ost->st, &pkt);
        if (ret < 0) {
            derror("Error while writing audio frame: %s\n",
                   avErr2Str(ret).c_str());
            return ret;
        }
    }

    return (frame || got_packet) ? 0 : 1;
}

// video output
static AVFrame* alloc_picture(enum AVPixelFormat pix_fmt,
                              int width,
                              int height) {
    AVFrame* picture;
    int ret;

    picture = av_frame_alloc();
    if (!picture)
        return NULL;

    picture->format = pix_fmt;
    picture->width = width;
    picture->height = height;

    // allocate the buffers for the frame data
    ret = av_frame_get_buffer(picture, 32);
    if (ret < 0) {
        derror("Could not allocate frame data.\n");
        return NULL;
    }

    return picture;
}

static int open_video(AVFormatContext* oc,
                      AVCodec* codec,
                      VideoOutputStream* ost,
                      AVDictionary* opt_arg) {
    int ret;
    AVCodecContext* c = ost->st->codec;
    AVDictionary* opt = NULL;

    av_dict_copy(&opt, opt_arg, 0);

    // open the codec
    ret = avcodec_open2(c, codec, &opt);
    av_dict_free(&opt);
    if (ret < 0) {
        derror("Could not open video codec: %s\n", avErr2Str(ret).c_str());
        return ret;
    }

    // allocate and init a re-usable frame
    ost->frame = alloc_picture(c->pix_fmt, c->width, c->height);
    if (!ost->frame) {
        derror("Could not allocate video frame\n");
        return -1;
    }

    // If the output format is not YUV420P, then a temporary YUV420P
    // picture is needed too. It is then converted to the required
    // output format.
    ost->tmp_frame = NULL;
    if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
        ost->tmp_frame = alloc_picture(AV_PIX_FMT_YUV420P, c->width, c->height);
        if (!ost->tmp_frame) {
            derror("Could not allocate temporary picture\n");
            return -1;
        }
    }

    return 0;
}

// encode one video frame and send it to the recorder
// return 1 when encoding is finished, 0 otherwise, negative if failed
static int write_video_frame(ffmpeg_recorder* recorder,
                             AVFormatContext* oc,
                             VideoOutputStream* ost,
                             AVFrame* frame) {
    int ret;
    AVCodecContext* c;
    int got_packet = 0;

    c = ost->st->codec;

    if (oc->oformat->flags & AVFMT_RAWPICTURE) {
        // a hack to avoid data copy with some raw video muxers
        AVPacket pkt;
        av_init_packet(&pkt);

        if (!frame)
            return 1;

        pkt.flags |= AV_PKT_FLAG_KEY;
        pkt.stream_index = ost->st->index;
        pkt.data = (uint8_t*)frame;
        pkt.size = sizeof(AVPicture);

        pkt.pts = pkt.dts = frame->pts;
        av_packet_rescale_ts(&pkt, c->time_base, ost->st->time_base);

        android::base::AutoLock lock(recorder->out_pkt_lock);
        ret = av_interleaved_write_frame(oc, &pkt);
    } else {
        AVPacket pkt = {0};
        av_init_packet(&pkt);

        // encode the frame
        ret = avcodec_encode_video2(c, &pkt, frame, &got_packet);
        if (ret < 0) {
            derror("Error encoding video frame: %s\n", avErr2Str(ret).c_str());
            return ret;
        }

        if (got_packet) {
            ret = write_frame(recorder, oc, &c->time_base, ost->st, &pkt);
        } else {
            ret = 0;
        }
    }

    if (ret < 0) {
        derror("Error while writing video frame: %s\n", avErr2Str(ret).c_str());
        return ret;
    }

    return (frame || got_packet) ? 0 : 1;
}

static void close_audio_stream(AVFormatContext* oc, AudioOutputStream* ost) {
    avcodec_close(ost->st->codec);
    av_frame_free(&ost->frame);
    av_frame_free(&ost->tmp_frame);
    swr_free(&ost->swr_ctx);
}

static void close_video_stream(AVFormatContext* oc, VideoOutputStream* ost) {
    avcodec_close(ost->st->codec);
    av_frame_free(&ost->frame);
    av_frame_free(&ost->tmp_frame);
    sws_freeContext(ost->sws_ctx);
}

ffmpeg_recorder* ffmpeg_create_recorder(const char* path) {
    static bool registered = false;
    ffmpeg_recorder* recorder = NULL;
    AVFormatContext* oc = NULL;

    if (path == NULL)
        return NULL;

    recorder = (ffmpeg_recorder*)malloc(sizeof(ffmpeg_recorder));
    if (recorder == NULL) {
        return NULL;
    }

    memset(recorder, 0, sizeof(ffmpeg_recorder));
    recorder->path = strdup(path);

    // Initialize libavcodec, and register all codecs and formats.
    if (!registered) {
        av_register_all();
        registered = true;
    }

    // allocate the output media context
    avformat_alloc_output_context2(&oc, NULL, NULL, path);
    if (oc == NULL) {
        free(recorder);
        return NULL;
    }

    recorder->oc = oc;
    recorder->start_time = android::base::System::get()->getHighResTimeUs();

    return recorder;
}

void ffmpeg_delete_recorder(ffmpeg_recorder* recorder) {
    if (recorder == NULL)
        return;

    // Write the trailer, if any. The trailer must be written before you
    // close the CodecContexts open when you wrote the header; otherwise
    // av_write_trailer() may try to use memory that was freed on
    // av_codec_close().
    av_write_trailer(recorder->oc);

    // Close each codec.
    if (recorder->have_video)
        close_video_stream(recorder->oc, &recorder->video_st);

    if (recorder->have_audio)
        close_audio_stream(recorder->oc, &recorder->audio_st);

    if (recorder->audio_capturer != NULL) {
        AudioCaptureEngine* engine = AudioCaptureEngine::get();
        if (engine != NULL)
            engine->stop(recorder->audio_capturer);
        delete recorder->audio_capturer;
    }

    if (recorder->audio_st.audio_leftover != nullptr) {
        free(recorder->audio_st.audio_leftover);
    }

    // Close the output file.
    if (!(recorder->oc->oformat->flags & AVFMT_NOFILE))
        avio_closep(&recorder->oc->pb);

    // free the stream
    avformat_free_context(recorder->oc);

    if (recorder->path)
        free(recorder->path);

    free(recorder);
}

// local helper to start the recording, after tracks are added
static int start_recording(ffmpeg_recorder* recorder) {
    int ret;
    AVDictionary* opt = NULL;

    if (recorder == NULL)
        return -1;

    if (recorder->started)
        return 0;

    AVFormatContext* oc = recorder->oc;

    av_dump_format(oc, 0, recorder->path, 1);

    AVOutputFormat* fmt = oc->oformat;

    // open the output file, if needed
    if (!(fmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&oc->pb, recorder->path, AVIO_FLAG_WRITE);
        if (ret < 0) {
            derror("Could not open '%s': %s\n", recorder->path,
                   avErr2Str(ret).c_str());
            free(recorder);
            return ret;
        }
    }

    // Write the stream header, if any.
    ret = avformat_write_header(oc, &opt);
    if (ret < 0) {
        derror("Error occurred when opening output file: %s\n",
               avErr2Str(ret).c_str());
        free(recorder);
        return ret;
    }

    VERBOSE_PRINT(capture, "recorder->audio_capturer=%p\n",
                  recorder->audio_capturer);
    if (recorder->audio_capturer != NULL) {
        AudioCaptureEngine* engine = AudioCaptureEngine::get();
        VERBOSE_PRINT(capture, "engine=%p\n", engine);
        if (engine != NULL)
            engine->start(recorder->audio_capturer);
    }

    recorder->started = true;
    return 0;
}

int ffmpeg_add_audio_track(ffmpeg_recorder* recorder,
                           int bit_rate,
                           int sample_rate) {
    if (recorder == NULL)
        return -1;

    AudioOutputStream* ost = &recorder->audio_st;
    enum AVCodecID codec_id = AV_CODEC_ID_AAC;
    AVFormatContext* oc = recorder->oc;
    AVOutputFormat* fmt = oc->oformat;

    fmt->video_codec = codec_id;

    ost->bit_rate = bit_rate;
    ost->sample_rate = sample_rate;

    // find the encoder
    AVCodec* codec = avcodec_find_encoder(codec_id);
    if (codec == NULL) {
        derror("Could not find encoder for '%s'\n", avcodec_get_name(codec_id));
        return -1;
    }

    ost->st = avformat_new_stream(oc, codec);
    if (!ost->st) {
        derror("Could not allocate stream\n");
        return -1;
    }

    ost->st->id = oc->nb_streams - 1;
    AVCodecContext* c = ost->st->codec;

    if (codec->type != AVMEDIA_TYPE_AUDIO) {
        return -1;
    }

    c->sample_fmt =
            codec->sample_fmts ? codec->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
    c->bit_rate = bit_rate;
    c->sample_rate = sample_rate;

    int i;
    if (codec->supported_samplerates) {
        c->sample_rate = codec->supported_samplerates[0];
        for (i = 0; codec->supported_samplerates[i]; i++) {
            if (codec->supported_samplerates[i] == sample_rate)
                c->sample_rate = sample_rate;
        }
    }
    c->channels = av_get_channel_layout_nb_channels(c->channel_layout);
    c->channel_layout = AV_CH_LAYOUT_STEREO;
    if (codec->channel_layouts) {
        c->channel_layout = codec->channel_layouts[0];
        for (i = 0; codec->channel_layouts[i]; i++) {
            if (codec->channel_layouts[i] == AV_CH_LAYOUT_STEREO)
                c->channel_layout = AV_CH_LAYOUT_STEREO;
        }
    }

    c->channels = av_get_channel_layout_nb_channels(c->channel_layout);
    ost->st->time_base = (AVRational){1, 1000000 /*c->sample_rate*/};

    c->time_base = ost->st->time_base;

    VERBOSE_PRINT(
            capture,
            "c->sample_fmt=%d, c->channels=%d, ost->st->time_base.den=%d\n",
            c->sample_fmt, c->channels, ost->st->time_base.den);

    // Some formats want stream headers to be separate.
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    recorder->have_audio = true;
    if (codec != NULL && (codec->capabilities & CODEC_CAP_EXPERIMENTAL) != 0) {
        ost->st->codec->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
    }

    AVDictionary* opt = NULL;
    int ret = open_audio(oc, codec, &recorder->audio_st, opt);

    // start audio capture, this informs QEMU to send audio to our muxer
    if (ret == 0 && recorder->audio_capturer == NULL) {
        recorder->audio_capturer =
                new FfmpegAudioCapturer(recorder, sample_rate, 16, 2);
    }

    return ret;
}

int ffmpeg_add_video_track(ffmpeg_recorder* recorder,
                           int width,
                           int height,
                           int bit_rate,
                           int fps) {
    if (recorder == NULL)
        return -1;

    VideoOutputStream* ost = &recorder->video_st;
    enum AVCodecID codec_id = AV_CODEC_ID_H264;
    AVFormatContext* oc = recorder->oc;
    AVOutputFormat* fmt = oc->oformat;

    fmt->video_codec = codec_id;

    ost->width = width;
    ost->height = height;
    ost->bit_rate = bit_rate;
    ost->fps = fps;

    // find the encoder
    AVCodec* codec = avcodec_find_encoder(codec_id);
    if (codec == NULL) {
        derror("Could not find encoder for '%s'\n", avcodec_get_name(codec_id));
        return -1;
    }

    ost->st = avformat_new_stream(oc, codec);
    if (!ost->st) {
        derror("Could not allocate stream\n");
        return -1;
    }

    ost->st->id = oc->nb_streams - 1;
    AVCodecContext* c = ost->st->codec;

    if (codec->type != AVMEDIA_TYPE_VIDEO) {
        return -1;
    }

    c->codec_id = codec_id;

    c->bit_rate = ost->bit_rate;
    // Resolution must be a multiple of two.
    c->width = ost->width;
    c->height = ost->height;

    // timebase: This is the fundamental unit of time (in seconds) in terms
    // of which frame timestamps are represented. For fixed-fps content,
    // timebase should be 1/framerate and timestamp increments should be
    // identical to 1.
    ost->st->time_base = (AVRational){1, 1000000};  // microsecond timebase

    c->time_base = ost->st->time_base;

    c->gop_size = 12;  // emit one intra frame every twelve frames at most
    c->pix_fmt = STREAM_PIX_FMT;
    if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
        // just for testing, we also add B frames
        c->max_b_frames = 2;
    }

    if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
        // Needed to avoid using macroblocks in which some coeffs overflow.
        // This does not happen with normal video, it just happens here as
        // the motion of the chroma plane does not match the luma plane.
        c->mb_decision = 2;
    }

    // Some formats want stream headers to be separate.
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    recorder->have_video = true;
    if (codec != NULL && (codec->capabilities & CODEC_CAP_EXPERIMENTAL) != 0) {
        ost->st->codec->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
    }

    AVDictionary* opt = NULL;
    int ret = open_video(oc, codec, ost, opt);

    return ret;
}

// Encode and write a video frame (in 32-bit RGBA format) to the recoder
// params:
//    recorder - the recorder instance
//    rgb_pixels - the byte array for the pixel in RGBA format, each pixel take
//    4 byte
//    size - the rgb_pixels array size, it should be exactly as 4 * width *
//    height
// return:
//   0    if successful
//   < 0  if failed
//
// this method is thread safe
int ffmpeg_encode_video_frame(ffmpeg_recorder* recorder,
                              const uint8_t* rgb_pixels,
                              int size) {
    if (recorder == NULL)
        return -1;

    int rc = -1;

    if (!recorder->started) {
        rc = start_recording(recorder);
        if (rc < 0)
            return rc;
    }

    VideoOutputStream* ost = &recorder->video_st;
    AVCodecContext* c = ost->st->codec;

    uint64_t elapseUS = android::base::System::get()->getHighResTimeUs() -
                        recorder->start_time;

    if (ost->sws_ctx == NULL) {
        ost->sws_ctx = sws_getContext(c->width, c->height, AV_PIX_FMT_RGBA,
                                      c->width, c->height, c->pix_fmt,
                                      SCALE_FLAGS, NULL, NULL, NULL);
        if (ost->sws_ctx == NULL) {
            derror("Could not initialize the conversion context\n");
            return -1;
        }
    }

    const int linesize[1] = {4 * c->width};
    sws_scale(ost->sws_ctx, (const uint8_t* const*)&rgb_pixels, linesize, 0,
              c->height, ost->frame->data, ost->frame->linesize);

    ost->frame->pts =
            (int64_t)(((double)elapseUS * ost->st->time_base.den) / 1000000.00);
    rc = write_video_frame(recorder, recorder->oc, ost, ost->frame);

    return rc;
}

// Encode and write a video frame (in 32-bit RGBA format) to the recoder
// params:
//    recorder - the recorder instance
//    buffer - the byte array for the audio buffer in PCM format
//    size - the audio buffer size
// return:
//   0    if successful
//   < 0  if failed
//
// this method is thread safe
int ffmpeg_encode_audio_frame(ffmpeg_recorder* recorder,
                              uint8_t* buffer,
                              int size) {
    if (recorder == NULL)
        return -1;

    int rc = -1;

    if (!recorder->started) {
        rc = start_recording(recorder);
        if (rc < 0)
            return rc;
    }

    AudioOutputStream* ost = &recorder->audio_st;
    if (ost->st == NULL)
        return -1;

    if (recorder->oc == NULL)
        return -1;

    uint64_t elapseUS = android::base::System::get()->getHighResTimeUs() -
                        recorder->start_time;

    int64_t pts =
            (int64_t)(((double)elapseUS * ost->st->time_base.den) / 1000000.00);

    AVFrame* frame = ost->tmp_frame;
    int frame_size = frame->nb_samples * ost->st->codec->channels *
                     sizeof(int16_t);  // 16-bit

    // we need to split into frames
    int remaining = size;
    uint8_t* buf = buffer;

    if (ost->audio_leftover_len > 0) {
        if (ost->audio_leftover_len + size >= frame_size) {
            memcpy(ost->audio_leftover + ost->audio_leftover_len, buf,
                   frame_size - ost->audio_leftover_len);
            memcpy(frame->data[0], ost->audio_leftover, frame_size);
            frame->pts = pts;
            write_audio_frame(recorder, recorder->oc, ost, frame);
            buf += (frame_size - ost->audio_leftover_len);
            remaining -= (frame_size - ost->audio_leftover_len);
            ost->audio_leftover_len = 0;
        } else {  // not enough for one frame yet
            memcpy(ost->audio_leftover + ost->audio_leftover_len, buf, size);
            ost->audio_leftover_len += size;
            remaining = 0;
        }
    }

    while (remaining >= frame_size) {
        memcpy(frame->data[0], buf, frame_size);
        frame->pts = pts;
        write_audio_frame(recorder, recorder->oc, ost, frame);
        buf += frame_size;
        remaining -= frame_size;
    }

    if (remaining > 0) {
        if (ost->audio_leftover == NULL)
            ost->audio_leftover = (uint8_t*)malloc(frame_size);

        memcpy(ost->audio_leftover, buf, remaining);
        ost->audio_leftover_len = remaining;
    }

    return 0;
}
