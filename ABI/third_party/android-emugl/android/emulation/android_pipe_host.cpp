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

#include "android/emulation/android_pipe_host.h"

#include "android/emulation/AndroidPipe.h"

namespace android {

using CStream = ::Stream;
using BaseStream = ::android::base::Stream;
using Service = AndroidPipe::Service;

namespace {

static CStream* asCStream(BaseStream* stream) {
    return reinterpret_cast<CStream*>(stream);
}

// An AndroidPipe derived class modeled around a legacy AndroidPipeFuncs
// structure.
class LegacyPipe : public AndroidPipe {
public:
    LegacyPipe(void* instance, void* hwPipe, Service* service)
        : AndroidPipe(hwPipe, service), mInstance(instance) {}

    const AndroidPipeFuncs* getFuncs() const;

    virtual void onGuestClose(PipeCloseReason reason) override {
        auto funcs = getFuncs();
        if (funcs->close) {
            funcs->close(mInstance);
        }
        // Delete the LegacyPipe instance, whether |mInstance| is deleted
        // immediately or not depends on the close() callback above.
        delete this;
    }

    virtual unsigned onGuestPoll() const { return getFuncs()->poll(mInstance); }

    virtual int onGuestRecv(AndroidPipeBuffer* buffers,
                            int numBuffers) override {
        return getFuncs()->recvBuffers(mInstance, buffers, numBuffers);
    }

    virtual int onGuestSend(const AndroidPipeBuffer* buffers,
                            int numBuffers) override {
        return getFuncs()->sendBuffers(mInstance, buffers, numBuffers);
    }

    virtual void onGuestWantWakeOn(int flags) override {
        getFuncs()->wakeOn(mInstance, flags);
    }

    virtual void onSave(BaseStream* stream) {
        if (getFuncs()->save) {
            getFuncs()->save(mInstance, asCStream(stream));
        }
    }

private:
    void* mInstance;
};

// An un-named service for legacy pipes.
class LegacyPipeService : public Service {
public:
    LegacyPipeService(const char* name,
                      void* opaque,
                      const AndroidPipeFuncs* funcs)
        : Service(name), mOpaque(opaque), mFuncs(funcs) {}

    virtual AndroidPipe* create(void* hwPipe, const char* args) override {
        void* hostPipe = mFuncs->init(hwPipe, mOpaque, args);
        if (!hostPipe) {
            return nullptr;
        }
        return new LegacyPipe(hostPipe, hwPipe, this);
    }

    virtual bool canLoad() const override { return (mFuncs->load != nullptr); }

    virtual AndroidPipe* load(void* hwPipe,
                              const char* args,
                              BaseStream* stream) override {
        void* instance = mFuncs->load(hwPipe, mOpaque, args, asCStream(stream));
        if (!instance) {
            return nullptr;
        }

        return new LegacyPipe(instance, hwPipe, this);
    }

    void* mOpaque;
    const AndroidPipeFuncs* mFuncs;
};

const AndroidPipeFuncs* LegacyPipe::getFuncs() const {
    return reinterpret_cast<const LegacyPipeService*>(mService)->mFuncs;
}

}  // namespace

}  // namespace android

void android_pipe_add_type(const char* pipeName,
                           void* pipeOpaque,
                           const AndroidPipeFuncs* pipeFuncs) {
    auto service =
            new android::LegacyPipeService(pipeName, pipeOpaque, pipeFuncs);
    android::Service::add(service);
}

void android_pipe_reset_service(void) {
    android::Service::resetAll();
}
