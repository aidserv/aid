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

#include "android/base/sockets/SocketUtils.h"

#include "android/base/sockets/ScopedSocket.h"
#include <gtest/gtest.h>

#include <errno.h>
#include <signal.h>
#include <string.h>

namespace android {
namespace base {

TEST(SocketUtils, socketSendAll) {
    char kData[] = "Hello World!";
    const size_t kDataLen = sizeof(kData) - 1U;

    int sock[2];
    ASSERT_EQ(0, socketCreatePair(&sock[0], &sock[1]));

    EXPECT_TRUE(socketSendAll(sock[0], kData, kDataLen));

    char data[kDataLen] = {};
    for (size_t n = 0; n < kDataLen; ++n) {
        EXPECT_EQ(1, socketRecv(sock[1], &data[n], 1)) << "#" << n;
        EXPECT_EQ(kData[n], data[n]) << "#" << n;
    }

    socketClose(sock[1]);
    socketClose(sock[0]);
}

TEST(SocketUtils, socketRecvAll) {
    char kData[] = "Hello World!";
    const size_t kDataLen = sizeof(kData) - 1U;

    int sock[2];
    ASSERT_EQ(0, socketCreatePair(&sock[0], &sock[1]));

    for (size_t n = 0; n < kDataLen; ++n) {
        EXPECT_EQ(1, socketSend(sock[0], &kData[n], 1)) << "#" << n;
    }

    char data[kDataLen] = {};
    EXPECT_TRUE(socketRecvAll(sock[1], data, kDataLen));

    for (size_t n = 0; n < kDataLen; ++n) {
        EXPECT_EQ(kData[n], data[n]) << "#" << n;
    }

    socketClose(sock[1]);
    socketClose(sock[0]);
}

TEST(SocketUtils, socketGetPort) {
    ScopedSocket s0;
    // Find a free TCP IPv4 port and bind to it.
    int port;
    for (port = 1024; port < 65536; ++port) {
        s0.reset(socketTcp4LoopbackServer(port));
        if (s0.valid()) {
            break;
        }
    }
    ASSERT_TRUE(s0.valid()) << "Could not find free TCP port";
    EXPECT_EQ(port, socketGetPort(s0.get()));

    // Connect to it.
    ScopedSocket s1(socketTcp4LoopbackClient(port));
    ASSERT_TRUE(s1.valid());

    // Accept it to create a pair of connected socket.
    // The port number of |s2| should be |port|.
    ScopedSocket s2(socketAcceptAny(s0.get()));
    ASSERT_TRUE(s2.valid());

    EXPECT_EQ(port, socketGetPort(s2.get()));
}

TEST(SocketUtils, socketGetPeerPort) {
    // Create a server socket bound to a random port.
    ScopedSocket s0(socketTcp4LoopbackServer(0));
    ASSERT_TRUE(s0.valid());
    int serverPort = socketGetPort(s0.get());
    ASSERT_GT(serverPort, 0);

    // Connect to it.
    ScopedSocket s1(socketTcp4LoopbackClient(serverPort));
    ASSERT_TRUE(s1.valid()) << "Could not connect to server socket";

    // Accept it to create a pair of connected sockets.
    ScopedSocket s2(socketAcceptAny(s0.get()));
    ASSERT_TRUE(s2.valid());

    EXPECT_EQ(socketGetPort(s1.get()), socketGetPeerPort(s2.get()));
    EXPECT_EQ(socketGetPort(s2.get()), socketGetPeerPort(s1.get()));
}

#ifndef _WIN32
namespace {

// Helper class to temporary install a SIGPIPE signal handler that
// will simply store the signal number into a global variable.
class SigPipeSignalHandler {
public:
    SigPipeSignalHandler() {
        sSignal = -1;
        struct sigaction act = {};
        act.sa_handler = myHandler;
        ::sigaction(SIGPIPE, &act, &mOldAction);
    }

    ~SigPipeSignalHandler() {
        ::sigaction(SIGPIPE, &mOldAction, nullptr);
    }

    int signaled() const { return sSignal; }

private:
    struct sigaction mOldAction;

    static int sSignal;

    static void myHandler(int sig) { sSignal = sig; }
};

// static
int SigPipeSignalHandler::sSignal = 0;

}  // namespace

TEST(SocketUtils, socketSendDoesNotGenerateSigPipe) {
    // Check that writing to a broken pipe does not generate a SIGPIPE
    // signal on non-Windows platforms.

    // First, check that this is true for socket pairs.
    {
        SigPipeSignalHandler handler;
        int s1, s2;
        EXPECT_EQ(-1, handler.signaled());
        ASSERT_EQ(0, socketCreatePair(&s1, &s2));
        socketClose(s2);
        errno = 0;
        EXPECT_EQ(-1, socketSend(s1, "x", 1));
        EXPECT_EQ(EPIPE, errno);
        EXPECT_EQ(-1, handler.signaled());
        socketClose(s1);
    }

    // Second, check that this is true for random TCP sockets.
    {
        SigPipeSignalHandler handler;
        EXPECT_EQ(-1, handler.signaled());
        ScopedSocket s1(socketTcp4LoopbackServer(0));  // Bind to random port
        ASSERT_TRUE(s1.valid());

        int port = socketGetPort(s1.get());
        ScopedSocket s2(socketTcp4LoopbackClient(port));
        ASSERT_TRUE(s2. valid());

        ScopedSocket s3(socketAcceptAny(s1.get()));
        ASSERT_TRUE(s3.valid());

        // s2 and s3 are now connected. Close s2 immediately, then try to
        // send data through s3.
        socketShutdownReads(s2.get());
        s2.close();

        // The EPIPE might not happen on the first send due to
        // TCP packet buffering in the kernel. Perform multiple send()
        // in a loop to work-around this.
        errno = 0;
        const int kMaxSendCount = 100;
        int n = 0;
        while (n < kMaxSendCount) {
            int ret = socketSend(s3.get(), "xxxx", 4);
            if (ret < 0) {
#ifdef __APPLE__
                // On OS X, errno is sometimes EPROTOTYPE instead of EPIPE
                // when this happens.
                EXPECT_TRUE(errno == EPIPE || errno == EPROTOTYPE)
                        << strerror(errno);
#else
                EXPECT_EQ(EPIPE, errno) << strerror(errno);
#endif
                break;
            }
            n++;
        }
        // For the record, the value of |n| would typically be 2 on Linux,
        // and 9 or 10 on Darwin, when run on local workstations.
        EXPECT_LT(n, kMaxSendCount);

        EXPECT_EQ(-1, handler.signaled());
    }
}
#endif  // !_WIN32

}  // namespace base
}  // namespace android
