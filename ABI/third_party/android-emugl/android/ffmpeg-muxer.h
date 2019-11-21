// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

///
// ffmpeg-muxer.h
//
// a ffmpeg based muxer, mp4 container format, H264 video format and AAC audio
// format
//
// example use:
//
//    ffmpeg_recorder *recorder = ffmpeg_create_recorder("~/test.mp4");
//    ffmpeg_add_video_track(recorder, 1280, 720, 512*1024*1024, 30);
//    ffmpeg_add_audio_track(recorder, 64*1024, 48000);
//    ffmpeg_encode_video_frame(recorder, rgb, size);
//    ffmpeg_encode_audio_frame(recorder, audio, audio_buf_size);
//    ...
//    ffmpeg_delete_recorder(recorder);
//

#pragma once

#include "android/utils/compiler.h"

#include <stdbool.h>
#include <stdint.h>

ANDROID_BEGIN_HEADER

typedef struct ffmpeg_recorder ffmpeg_recorder;

// Create an instance of the ffmpeg recorder (mp4 container format)
// params:
//   path - the path to save the generated video file which should have .mp4
//   extension
// return:
//   opaque pointer to the recorder struct, which must be freed by calling
//   ffmpeg_delete_recorder()
//   NULL if failed
//
// this method is thread safe
ffmpeg_recorder *ffmpeg_create_recorder(const char *path);

//
// Save the output file and delete the recorder, this method must be called.
// params:
//  recorder: the recorder pointer returned from ffmpeg_create_recorder()
//
// this method is thread safe
void ffmpeg_delete_recorder(ffmpeg_recorder *recorder);

// Add an audio track from the specified format, audio track is optional
// stero audio and PCM format are assumed
// params:
//   recorder - the recorder instance
//   bit_rate - the audio recording bit rate, the higher this number is, the
//   better quality and the larger video file size,
//              for example, 64000
//   sample_rate - usually 48000 (system audio) or 44100 (mic)
// return:
//   0    if successful
//   < 0  if failed
//
// this method is thread safe
int ffmpeg_add_audio_track(ffmpeg_recorder *recorder, int bit_rate,
                           int sample_rate);

// Add a video track from the specified format, video track is required in
// order to generate a correct mp4 file
// params:
//   recorder - the recorder instance
//   width/height - the video size which determine the rgb pixel array size
//   used in ffmpeg_encode_video_frame() method
//   bit_rate - the video recording bit rate, the higher this number is, the
//   better the quality and the larger video file size.
//              for example, 512 * 1024 * 1024. It should be higher than audio
//              bit rate
//   fps - frame rate per second, 30 and 60 are good numbers
// return:
//   0    if successful
//   < 0  if failed
//
// this method is thread safe
int ffmpeg_add_video_track(ffmpeg_recorder *recorder, int width, int height,
                           int bit_rate, int fps);

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
int ffmpeg_encode_audio_frame(ffmpeg_recorder *recorder, uint8_t *buffer, 
                              int size);

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
int ffmpeg_encode_video_frame(ffmpeg_recorder *recorder,
                              const uint8_t *rgb_pixels, int size);

ANDROID_END_HEADER
