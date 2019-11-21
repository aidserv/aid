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

#include "android/emulation/AdbDebugPipe.h"

#include <stdio.h>

namespace android {
namespace emulation {

AndroidPipe* AdbDebugPipe::Service::create(void* hwPipe, const char* args) {
    return new AdbDebugPipe(hwPipe, this, mOutput.get());
}

bool AdbDebugPipe::Service::canLoad() const {
    // canLoad() returns true to avoid force-closing the pipe on
    // restore. However, there is no state to save proper :)
    return true;
}

AndroidPipe* AdbDebugPipe::Service::load(void* hwPipe,
                                         const char* args,
                                         android::base::Stream* stream) {
    // No state to load, just create new instance.
    return create(hwPipe, args);
}

void AdbDebugPipe::onGuestClose(PipeCloseReason reason) {
    // Nothing to do here
}

unsigned AdbDebugPipe::onGuestPoll() const {
    // Guest can always write and never read.
    return PIPE_POLL_OUT;
}

int AdbDebugPipe::onGuestRecv(AndroidPipeBuffer* buffers, int count) {
    // Guest can never read.
    return PIPE_ERROR_IO;
}

int AdbDebugPipe::onGuestSend(const AndroidPipeBuffer* buffers,
                              int numBuffers) {
    int result = 0;
    while (numBuffers > 0) {
        if (mOutput) {
            mOutput->write(buffers->data, buffers->size);
        }
        result += static_cast<int>(buffers->size);
        buffers++;
        numBuffers--;
    }
    return result;
}

void AdbDebugPipe::onGuestWantWakeOn(int flags) {
    // Nothing to do here, since onGuestSend() never returns
    // PIPE_ERROR_AGAIN.
}

void AdbDebugPipe::onSave(android::base::Stream* stream) {
    // No state to save.
}

}  // namespace emulation
}  // namespace android
