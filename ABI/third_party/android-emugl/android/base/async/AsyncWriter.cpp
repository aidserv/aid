#include "android/base/async/AsyncWriter.h"

#include "android/base/sockets/SocketErrors.h"
#include "android/base/sockets/SocketUtils.h"

namespace android {
namespace base {

void AsyncWriter::reset(const void* buffer,
                        size_t bufferSize,
                        Looper::FdWatch* watch) {
    mBuffer = reinterpret_cast<const uint8_t*>(buffer);
    mBufferSize = bufferSize;
    mPos = 0;
    mFdWatch = watch;
    if (bufferSize > 0) {
        watch->wantWrite();
    }
}

AsyncStatus AsyncWriter::run() {
    if (mPos >= mBufferSize) {
        return kAsyncCompleted;
    }

    do {
        ssize_t ret = socketSend(mFdWatch->fd(),
                                 mBuffer + mPos,
                                 mBufferSize - mPos);
        if (ret == 0) {
            // Disconnection!
            errno = ECONNRESET;
            return kAsyncError;
        }
        if (ret < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                return kAsyncAgain;
            }
            return kAsyncError;
        }
        mPos += static_cast<size_t>(ret);
    } while (mPos < mBufferSize);

    mFdWatch->dontWantWrite();
    return kAsyncCompleted;
}

}  // namespace base
}  // namespace android
