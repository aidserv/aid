// Copyright 2014 The Android Open Source Project
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

#include "android/base/async/AsyncStatus.h"
#include "android/base/async/Looper.h"

#include <stdint.h>

namespace android {
namespace base {

class AsyncWriter {
public:
    AsyncWriter() :
            mBuffer(NULL),
            mBufferSize(0U),
            mPos(0U),
            mFdWatch(NULL) {}

    void reset(const void* buffer,
               size_t bufferSize,
               Looper::FdWatch* watch);

    AsyncStatus run();

private:
    const uint8_t* mBuffer;
    size_t mBufferSize;
    size_t mPos;
    Looper::FdWatch* mFdWatch;
};

}  // namespace base
}  // namespace android
