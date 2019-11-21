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

#include "android/base/Log.h"
#include "android/base/sockets/ScopedSocket.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/base/StringView.h"
#include "android/base/threads/Thread.h"

#include <string>

#include <stddef.h>

#ifdef _WIN32
#undef ERROR
#endif

namespace android {
namespace base {

// Simple server thread that receives data and stores it in a buffer.
// Usage is the following:
//
// 1) Create instance, passing a port to listen to the constructor.
//    The default constructor will bind to a random port, which you
//    can retrieve with the port() method. Check that the port could be
//    be bound with the valid() method.
//
// 2) Start the thread, it will store all the data it receives into
//    an internal buffer. The thread only stops when the connection
//    is closed from the remote end.
//
// 3) Join the thread, the result is the size of the buffer in bytes, or
//    -1 if a connection could not be accepted.
//
// 4) Use view() to retrieve a view of the content.
//
class TestInputBufferSocketServerThread : public android::base::Thread {
public:
    // Create new thread instance, try to bound to specific TCP |port|,
    // a value of 0 let the system choose a free IPv4 port, which can
    // later be retrieved with port().
    TestInputBufferSocketServerThread(int port = 0)
        : Thread(), mSocket(android::base::socketTcp4LoopbackServer(port)) {}

    // Returns true if port could be bound.
    bool valid() const { return mSocket.valid(); }

    // Return bound server port.
    int port() const { return android::base::socketGetPort(mSocket.get()); }

    // Return buffer content as a StringView.
    StringView view() const { return mString; }

    // Main function simply receives everything and stores it in a string.
    virtual intptr_t main() override {
        // Wait for a single connection.
        int fd = android::base::socketAcceptAny(mSocket.get());
        if (fd < 0) {
            LOG(ERROR) << "Could not accept one connection!";
            return -1;
        }

        // Now read data from the connection until it closes.
        size_t size = 0;
        for (;;) {
            size_t capacity = mString.size();
            if (size >= capacity) {
                mString.resize(1024 + capacity * 2);
                capacity = mString.size();
            }
            size_t avail = capacity - size;
            ssize_t len = android::base::socketRecv(fd, &mString[size], avail);
            if (len <= 0) {
                break;
            }
            size += len;
        }
        android::base::socketClose(fd);

        mString.resize(size);
        return static_cast<intptr_t>(size);
    }

private:
    ScopedSocket mSocket;
    std::string mString;
};

}  // namespace base
}  // namespace android
