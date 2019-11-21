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
#include "android/opengl/OpenglEsPipe.h"

#include "android/base/async/Looper.h"
#include "android/base/files/StreamSerializing.h"
#include "android/opengles.h"
#include "android/opengl-snapshot.h"
#include "android/opengles-pipe.h"
#include "android/opengl/GLProcessPipe.h"

#ifdef SNAPSHOT_PROFILE
#include "android/base/system/System.h"
#endif

#include <atomic>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

// Set to 1 or 2 for debug traces
#define DEBUG 0

#if DEBUG >= 1
#define D(...) printf(__VA_ARGS__), printf("\n"), fflush(stdout)
#else
#define D(...) ((void)0)
#endif

#if DEBUG >= 2
#define DD(...) printf(__VA_ARGS__), printf("\n"), fflush(stdout)
#else
#define DD(...) ((void)0)
#endif

using ChannelBuffer = emugl::RenderChannel::Buffer;
using emugl::RenderChannel;
using emugl::RenderChannelPtr;
using ChannelState = emugl::RenderChannel::State;
using IoResult = emugl::RenderChannel::IoResult;

#define OPENGL_SAVE_VERSION 1

namespace android {
namespace opengl {

namespace {

class EmuglPipe : public AndroidPipe {
public:
    //////////////////////////////////////////////////////////////////////////
    // The pipe service class for this implementation.
    class Service : public AndroidPipe::Service {
    public:
        Service() : AndroidPipe::Service("opengles") {}

        // Create a new EmuglPipe instance.
        AndroidPipe* create(void* hwPipe, const char* args) override {
            return createPipe(hwPipe, this, args);
        }

        bool canLoad() const override { return true; }

        virtual void preLoad(android::base::Stream* stream) override {
#ifdef SNAPSHOT_PROFILE
            mLoadStartTime = android::base::System::get()->getUnixTimeUs();
#endif
            const bool hasRenderer = stream->getByte();
            const auto& renderer = android_getOpenglesRenderer();
            if (hasRenderer != (bool)renderer) {
                // die?
                return;
            }
            if (!hasRenderer) {
                return;
            }
            int version = stream->getBe32();
            (void)version;
            renderer->load(stream);
#ifdef SNAPSHOT_PROFILE
            printf("OpenglEs preload time: %ld ms\n",
                    (android::base::System::get()->getUnixTimeUs()
                    - mLoadStartTime) / 1000);
#endif
        }

        void postLoad(android::base::Stream* stream) override {
            if (const auto& renderer = android_getOpenglesRenderer()) {
                renderer->resumeAll();
            }
#ifdef SNAPSHOT_PROFILE
            printf("OpenglEs total load time: %ld ms\n",
                    (android::base::System::get()->getUnixTimeUs()
                    - mLoadStartTime) / 1000);
#endif
        }

        void preSave(android::base::Stream* stream) override {
#ifdef SNAPSHOT_PROFILE
            mSaveStartTime = android::base::System::get()->getUnixTimeUs();
#endif
            if (const auto& renderer = android_getOpenglesRenderer()) {
                renderer->pauseAllPreSave();
                stream->putByte(1);
                stream->putBe32(OPENGL_SAVE_VERSION);
                renderer->save(stream);
            } else {
                stream->putByte(0);
            }
        }

        void postSave(android::base::Stream* stream) override {
            if (const auto& renderer = android_getOpenglesRenderer()) {
                renderer->resumeAll();
            }
#ifdef SNAPSHOT_PROFILE
            printf("OpenglEs total save time: %ld ms\n",
                    (android::base::System::get()->getUnixTimeUs()
                    - mSaveStartTime) / 1000);
#endif
        }

        virtual AndroidPipe* load(void* hwPipe,
                              const char* args,
                              android::base::Stream* stream) override {
            return createPipe(hwPipe, this, args, stream);
        }

    private:
        static AndroidPipe* createPipe(
                void* hwPipe, Service* service,
                const char* args, android::base::Stream* loadStream = nullptr) {
            const auto& renderer = android_getOpenglesRenderer();
            if (!renderer) {
                // This should never happen, unless there is a bug in the
                // emulator's initialization, or the system image, or we're
                // loading from an incompatible snapshot.
                D("Trying to open the OpenGLES pipe without GPU emulation!");
                return nullptr;
            }

            auto pipe = new EmuglPipe(hwPipe, service, renderer, loadStream);
            if (!pipe->mIsWorking) {
                delete pipe;
                pipe = nullptr;
            }
            return pipe;
        }
#ifdef SNAPSHOT_PROFILE
        android::base::System::Duration mSaveStartTime = 0;
        android::base::System::Duration mLoadStartTime = 0;
#endif
    };

    /////////////////////////////////////////////////////////////////////////
    // Constructor, check that |mIsWorking| is true after this call to verify
    // that everything went well.
    EmuglPipe(void* hwPipe, Service* service,
              const emugl::RendererPtr& renderer,
              android::base::Stream* loadStream = nullptr)
        : AndroidPipe(hwPipe, service) {
        printf("EmuglPipe\n");
        bool isWorking = true;
        if (loadStream) {
            DD("%s: loading GLES pipe state for hwpipe=%p", __func__, mHwPipe);
            isWorking = (bool)loadStream->getBe32();
            android::base::loadBuffer(loadStream, &mDataForReading);
            mDataForReadingLeft = loadStream->getBe32();
        }

        mChannel = renderer->createRenderChannel(loadStream);
        if (!mChannel) {
            D("Failed to create an OpenGLES pipe channel!");
            return;
        }

        mIsWorking = isWorking;
        mChannel->setEventCallback(
                [this](RenderChannel::State events) {
                    onChannelHostEvent(events);
                });
    }

    //////////////////////////////////////////////////////////////////////////
    // Overriden AndroidPipe methods

    virtual void onSave(android::base::Stream* stream) override {
        DD("%s: saving GLES pipe state for hwpipe=%p", __FUNCTION__, mHwPipe);
        stream->putBe32(mIsWorking);
        android::base::saveBuffer(stream, mDataForReading);
        stream->putBe32(mDataForReadingLeft);

        mChannel->onSave(stream);
    }

    virtual void onGuestClose(PipeCloseReason reason) override {
        D("%s", __func__);
        mIsWorking = false;
        mChannel->stop();
        // Make sure there's no operation scheduled for this pipe instance to
        // run on the main thread.
        abortPendingOperation();
        delete this;
    }

    virtual unsigned onGuestPoll() const override {
        DD("%s", __func__);

        unsigned ret = 0;
        if (mDataForReadingLeft > 0) {
            ret |= PIPE_POLL_IN;
        }
        ChannelState state = mChannel->state();
        if ((state & ChannelState::CanRead) != 0) {
            ret |= PIPE_POLL_IN;
        }
        if ((state & ChannelState::CanWrite) != 0) {
            ret |= PIPE_POLL_OUT;
        }
        if ((state & ChannelState::Stopped) != 0) {
            ret |= PIPE_POLL_HUP;
        }
        DD("%s: returning %d", __func__, ret);
        return ret;
    }

    virtual int onGuestRecv(AndroidPipeBuffer* buffers, int numBuffers)
            override {
        DD("%s", __func__);

        // Consume the pipe's dataForReading, then put the next received data
        // piece there. Repeat until the buffers are full or we're out of data
        // in the channel.
        int len = 0;
        size_t buffOffset = 0;

        auto buff = buffers;
        const auto buffEnd = buff + numBuffers;
        while (buff != buffEnd) {
            if (mDataForReadingLeft == 0) {
                // No data left, read a new chunk from the channel.
                int spinCount = 20;
                for (;;) {
                    auto result = mChannel->tryRead(&mDataForReading);
                    if (result == IoResult::Ok) {
                        mDataForReadingLeft = mDataForReading.size();
                        break;
                    }
                    DD("%s: tryRead() failed with %d", __func__, (int)result);
                    // This failed either because the channel was stopped
                    // from the host, or if there was no data yet in the
                    // channel.
                    if (len > 0) {
                        DD("%s: returning %d bytes", __func__, (int)len);
                        return len;
                    }
                    if (result == IoResult::Error) {
                        return PIPE_ERROR_IO;
                    }
                    // Spin a little before declaring there is nothing
                    // to read. Many GL calls are much faster than the
                    // whole host-to-guest-to-host transition.
                    if (--spinCount > 0) {
                        continue;
                    }
                    DD("%s: returning PIPE_ERROR_AGAIN", __func__);
                    return PIPE_ERROR_AGAIN;
                }
            }

            const size_t curSize =
                    std::min(buff->size - buffOffset, mDataForReadingLeft);
            memcpy(buff->data + buffOffset,
                mDataForReading.data() +
                        (mDataForReading.size() - mDataForReadingLeft),
                curSize);

            len += curSize;
            mDataForReadingLeft -= curSize;
            buffOffset += curSize;
            if (buffOffset == buff->size) {
                ++buff;
                buffOffset = 0;
            }
        }

        DD("%s: received %d bytes", __func__, (int)len);
        return len;
    }

    virtual int onGuestSend(const AndroidPipeBuffer* buffers,
                            int numBuffers) override {
        DD("%s", __func__);

        if (!mIsWorking) {
            DD("%s: pipe already closed!", __func__);
            return PIPE_ERROR_IO;
        }

        // Count the total bytes to send.
        int count = 0;
        for (int n = 0; n < numBuffers; ++n) {
            count += buffers[n].size;
        }

        // Copy everything into a single ChannelBuffer.
        ChannelBuffer outBuffer;
        outBuffer.resize_noinit(count);
        auto ptr = outBuffer.data();
        for (int n = 0; n < numBuffers; ++n) {
            memcpy(ptr, buffers[n].data, buffers[n].size);
            ptr += buffers[n].size;
        }

        D("%s: sending %d bytes to host", __func__, count);
        // Send it through the channel.
        auto result = mChannel->tryWrite(std::move(outBuffer));
        if (result != IoResult::Ok) {
            D("%s: tryWrite() failed with %d", __func__, (int)result);
            return result == IoResult::Error ? PIPE_ERROR_IO : PIPE_ERROR_AGAIN;
        }

        return count;
    }

    virtual void onGuestWantWakeOn(int flags) override {
        DD("%s: flags=%d", __func__, flags);

        // Translate |flags| into ChannelState flags.
        ChannelState wanted = ChannelState::Empty;
        if (flags & PIPE_WAKE_READ) {
            wanted |= ChannelState::CanRead;
        }
        if (flags & PIPE_WAKE_WRITE) {
            wanted |= ChannelState::CanWrite;
        }

        // Signal events that are already available now.
        ChannelState state = mChannel->state();
        ChannelState available = state & wanted;
        DD("%s: state=%d wanted=%d available=%d", __func__, (int)state,
           (int)wanted, (int)available);
        if (available != ChannelState::Empty) {
            DD("%s: signaling events %d", __func__, (int)available);
            signalState(available);
            wanted &= ~available;
        }

        // Ask the channel to be notified of remaining events.
        if (wanted != ChannelState::Empty) {
            DD("%s: waiting for events %d", __func__, (int)wanted);
            mChannel->setWantedEvents(wanted);
        }
    }

private:
    // Called to signal the guest that read/write wake events occured.
    // Note: this can be called from either the guest or host render
    // thread.
    void signalState(ChannelState state) {
        int wakeFlags = 0;
        if ((state & ChannelState::CanRead) != 0) {
            wakeFlags |= PIPE_WAKE_READ;
        }
        if ((state & ChannelState::CanWrite) != 0) {
            wakeFlags |= PIPE_WAKE_WRITE;
        }
        if (wakeFlags != 0) {
            this->signalWake(wakeFlags);
        }
    }

    // Called when an i/o event occurs on the render channel
    void onChannelHostEvent(ChannelState state) {
        D("%s: events %d (working %d)", __func__, (int)state, (int)mIsWorking);
        // NOTE: This is called from the host-side render thread.
        // but closeFromHost() and signalWake() can be called from
        // any thread.
        if (!mIsWorking) {
            return;
        }
        if ((state & ChannelState::Stopped) != 0) {
            this->closeFromHost();
            return;
        }
        signalState(state);
    }

    // A RenderChannel pointer used for communication.
    RenderChannelPtr mChannel;

    // Set to |true| if the pipe is in working state, |false| means we're not
    // initialized or the pipe is closed.
    bool mIsWorking = false;

    // These two variables serve as a reading buffer for the guest.
    // Each time we get a read request, first we extract a single chunk from
    // the |mChannel| into here, and then copy its content into the
    // guest-supplied memory.
    // If guest didn't have enough room for the whole buffer, we track the
    // number of remaining bytes in |mDataForReadingLeft| for the next read().
    ChannelBuffer mDataForReading;
    size_t mDataForReadingLeft = 0;

    DISALLOW_COPY_ASSIGN_AND_MOVE(EmuglPipe);
};

}  // namespace

void registerPipeService() {
    android::AndroidPipe::Service::add(new EmuglPipe::Service());
    registerGLProcessPipeService();
}

}  // namespace opengl
}  // namespace android

// Declared in android/opengles-pipe.h
void android_init_opengles_pipe() {
	printf("android_init_opengles_pipe\n");
    android::opengl::registerPipeService();
}

void android_init_opengles_tcp()
{
#if 0
	const auto& renderer = android_getOpenglesRenderer();
	if (!renderer) {
		// This should never happen, unless there is a bug in the
		// emulator's initialization, or the system image, or we're
		// loading from an incompatible snapshot.
		D("Trying to open the OpenGLES pipe           GPU emulation!");
		return ;
	}
#endif
	char addr[256];
	if (!android_StartOpenglServer(addr, 256))
	{
		
	}
	

}
