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

#include "android/base/async/AsyncReader.h"

#include "android/base/sockets/SocketErrors.h"
#include "android/base/sockets/SocketUtils.h"

namespace android {
namespace base {

void AsyncReader::reset(void* buffer,
                        size_t bufferSize,
                        Looper::FdWatch* watch) {
    mBuffer = reinterpret_cast<uint8_t*>(buffer);
    mBufferSize = bufferSize;
    mPos = 0U;
    mFdWatch = watch;
    if (bufferSize > 0) {
        watch->wantRead();
    }
}

AsyncStatus AsyncReader::run() {
    if (mPos >= mBufferSize) {
        return kAsyncCompleted;
    }

    do {
        ssize_t ret = socketRecv(mFdWatch->fd(),
                                 mBuffer + mPos,
                                 mBufferSize - mPos);
        if (ret == 0) {
            // Disconnection!
            errno = ECONNRESET;
            return kAsyncError;
        }

        if (ret < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                mFdWatch->wantRead();
                return kAsyncAgain;
            }
            return kAsyncError;
        }
        mPos += static_cast<size_t>(ret);
    } while (mPos < mBufferSize);

    mFdWatch->dontWantRead();
    return kAsyncCompleted;
}

}  // namespace base
}  // namespace android
