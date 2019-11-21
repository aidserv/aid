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
#pragma once

#include "android/emulation/AndroidPipe.h"

#include <fstream>

namespace android {
namespace emulation {

// This is a pipe for reading the logcat output from the guest.
// The other side is connected to Android's logcat service

class LogcatPipe final : public AndroidPipe {
public:

    class Service final : public AndroidPipe::Service {
    public:
        Service();
        AndroidPipe* create(void* hwPipe, const char* args) override;
    };

    LogcatPipe(void* hwPipe, Service* svc);

    void onGuestClose(PipeCloseReason reason) override;
    unsigned onGuestPoll() const override;
    int onGuestRecv(AndroidPipeBuffer* buffers, int numBuffers) override;
    int onGuestSend(const AndroidPipeBuffer* buffers, int numBuffers) override;
    void onGuestWantWakeOn(int flags) override {
    }
private:
    std::ofstream mOutputFile;
};

void registerLogcatPipeService();

}  // namespace emulation
}  // namespace android
