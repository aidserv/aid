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

#include "android/emulation/AndroidPipe.h"

#include "android/opengles.h"
#include <assert.h>
#include <atomic>
#include <memory>

namespace android {
namespace opengl {

namespace {

// GLProcessPipe is a pipe service that is used for releasing graphics resources
// per guest process. At the time being, guest processes can acquire host color
// buffer handles / EGLImage handles and they need to be properly released when
// guest process exits unexpectedly. This class is used to detect if guest
// process exits, so that a proper cleanup function can be called.

// It is done by setting up a pipe per guest process before acquiring color
// buffer handles. When guest process exits, the pipe will be closed, and
// onGuestClose() will trigger the cleanup path.

class GLProcessPipe : public AndroidPipe {
public:
    //////////////////////////////////////////////////////////////////////////
    // The pipe service class for this implementation.
    class Service : public AndroidPipe::Service {
    public:
        Service() : AndroidPipe::Service("GLProcessPipe") {}

        bool canLoad() const override { return true; }

        AndroidPipe* create(void* hwPipe, const char* args) override {
            return new GLProcessPipe(hwPipe, this);
        }

        AndroidPipe* load(void* hwPipe, const char* args,
                         base::Stream* stream) override {
            return new GLProcessPipe(hwPipe, this, stream);
        }

        void preLoad(base::Stream* stream) override {
            GLProcessPipe::s_headId.store(stream->getBe64());
        }

        void preSave(base::Stream* stream) override {
            stream->putBe64(GLProcessPipe::s_headId.load());
        }
    };

    GLProcessPipe(void* hwPipe, Service* service,
                  base::Stream* loadStream = nullptr)
        : AndroidPipe(hwPipe, service) {
        if (loadStream) {
            m_uniqueId = loadStream->getBe64();
        } else {
            m_uniqueId = ++s_headId;
        }
    }

    void onSave(base::Stream* stream) override {
        stream->putBe64(m_uniqueId);
    }

    void onGuestClose(PipeCloseReason reason) override {
        android_cleanupProcGLObjects(m_uniqueId);
    }

    unsigned onGuestPoll() const override {
        return PIPE_POLL_IN | PIPE_POLL_OUT;
    }

    int onGuestRecv(AndroidPipeBuffer* buffers, int numBuffers) override {
        assert(buffers[0].size >= 8);
        memcpy(buffers[0].data, (const char*)&m_uniqueId, sizeof(m_uniqueId));
        return sizeof(m_uniqueId);
    }

    int onGuestSend(const AndroidPipeBuffer* buffers,
                            int numBuffers) override {
        // The guest is supposed to send us a confirm code first. The code is
        // 100 (4 byte integer).
        assert(buffers[0].size >= 4);
        int32_t confirmInt = *((int32_t*)buffers[0].data);
        assert(confirmInt == 100);
        (void)confirmInt;
        return buffers[0].size;
    }

    void onGuestWantWakeOn(int flags) override {}

private:
    // An identifier for the guest process corresponding to this pipe.
    // With very high probability, all currently-active processes have unique
    // identifiers, since the IDs are assigned sequentially from a 64-bit ID
    // space.
    // Please change it if you ever have a use case that exhausts them
    uint64_t m_uniqueId;
    static std::atomic<uint64_t> s_headId;

};

std::atomic<uint64_t> GLProcessPipe::s_headId {0};

}

void registerGLProcessPipeService() {
    android::AndroidPipe::Service::add(new GLProcessPipe::Service());
}

}
}
