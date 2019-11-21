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

#pragma once

#include "android/base/files/Stream.h"
#include "android/emulation/AndroidPipe.h"

#include <memory>

namespace android {
namespace emulation {

// AdbDebugPipe implements pipes for the 'qemud:adb-debug' pipe service
// which simply prints ADB debug messages to an optional output stream.
//
// Usage:
//    1) Create a new AdbDebugPipe::Service instance, passing an
//       android::base::Stream instance, or nullptr to disable any output.
//
//    2) Call AndroidPipe::Service::add() to register it at emulation setup
//       time.
//
class AdbDebugPipe : public AndroidPipe {
public:
    using Stream = android::base::Stream;

    // Service implementation class.
    class Service : public AndroidPipe::Service {
    public:
        // Create new service instance. |output| is either nullptr
        // (to disable output), or a valid android::base::Stream
        // instance. NOTE: This transfers ownership of |output|
        // to the service.
        Service(Stream* output)
            : AndroidPipe::Service("qemud:adb-debug"), mOutput(output) {}

        virtual AndroidPipe* create(void* hwPipe, const char* args) override;

        virtual bool canLoad() const override;

        virtual AndroidPipe* load(void* hwPipe,
                                  const char* args,
                                  android::base::Stream* stream) override;

    private:
        std::unique_ptr<Stream> mOutput;
    };

    virtual void onGuestClose(PipeCloseReason reason) override;
    virtual unsigned onGuestPoll() const override;
    virtual int onGuestRecv(AndroidPipeBuffer* buffers, int count) override;
    virtual int onGuestSend(const AndroidPipeBuffer* buffers,
                            int count) override;

    virtual void onGuestWantWakeOn(int flags) override;
    virtual void onSave(android::base::Stream* stream) override;

private:
    AdbDebugPipe(void* hwPipe, Service* service, Stream* output)
        : AndroidPipe(hwPipe, service), mOutput(output) {}

    Stream* mOutput = nullptr;
};

}  // namespace emulation
}  // namespace android
