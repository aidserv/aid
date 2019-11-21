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

// Helper class to help read data from a socket asynchronously.
//
// Call reset() to indicate how many bytes of data to read into a
// client-provided buffer, and a Looper::FdWatch to use.
//
// Then in the FdWatch callback, call run() on i/o read events.
// The function returns an AsyncStatus that indicates what to do
// after the run.
//
// Usage example:
//
//     AsyncReader myReader;
//     myReader.reset(myBuffer, sizeof myBuffer, myFdWatch);
//     ...
//     // when an event happens on myFdWatch
//     AsyncStatus status = myReader.run();
//     if (status == kCompleted) {
//         // finished reading the data.
//     } else if (status == kError) {
//         // i/o error (i.e. socket disconnection).
//     } else {  // really |status == kAgain|
//         // still more data needed to fill the buffer.
//     }
//
class AsyncReader {
public:
    AsyncReader() :
            mBuffer(NULL),
            mBufferSize(0U),
            mPos(0),
            mFdWatch(NULL) {}

    void reset(void* buffer, size_t buffSize, Looper::FdWatch* watch);

    AsyncStatus run();

private:
    uint8_t* mBuffer;
    size_t mBufferSize;
    size_t mPos;
    Looper::FdWatch* mFdWatch;
};

}  // namespace base
}  // namespace android
