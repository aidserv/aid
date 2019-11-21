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

#include "android/emulation/AdbHostListener.h"

#include "android/base/async/ThreadLooper.h"
#include "android/base/Log.h"
#include "android/base/sockets/ScopedSocket.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/base/StringFormat.h"
#include "android/base/StringView.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/threads/Thread.h"
#include "android/base/testing/TestInputBufferSocketServerThread.h"

#include <gtest/gtest.h>

// Because gtest includes Windows headers that declare the ERROR
// macro that prevents LOG(ERROR) from compiling!!
#ifdef _WIN32
#undef ERROR
#endif

#include <memory>
#include <errno.h>

namespace android {
namespace emulation {

using android::base::AutoLock;
using android::base::Lock;
using android::base::ScopedSocket;
using android::base::StringFormat;
using android::base::ThreadLooper;
using android::base::TestInputBufferSocketServerThread;

namespace {

// A thread that will connect to a given TCP4 loopback port, send a 32-bit ID
// integer through it, then exit. Usage is:
//
//   1) Create intsance, passing a |port| and |id| value.
//   2) Start the thread.
//   3) Join/wait for the thread.
//
// It's possible to query the thread's state any time, and from any thread,
// by calling the state() method.
class TestConnectThread : public android::base::Thread {
public:
    enum State {
        kInit = 0,
        kConnected,
        kDisconnected,
    };

    explicit TestConnectThread(int port, int id) : mPort(port), mId(id) {}

    State state() const {
        AutoLock lock(mLock);
        return mState;
    }

    virtual intptr_t main() override {
        // Connect
        std::string prefix = StringFormat("[thread %d] ", mId);
        ScopedSocket fd(android::base::socketTcp4LoopbackClient(mPort));
        if (!fd.valid()) {
            PLOG(ERROR) << prefix << "Connection failed";
            return -1;
        }
        setState(kConnected);
        uint32_t id = static_cast<uint32_t>(mId);
        ssize_t len = android::base::socketSend(fd.get(), &id, sizeof(id));
        if (len == 0) {
            LOG(INFO) << prefix << "End of stream";
        }
        if (len < 0) {
            PLOG(ERROR) << prefix;
        }
        if (len != static_cast<ssize_t>(sizeof(id))) {
            LOG(INFO) << prefix << "Invalid sent byte count=" << len;
        }
        setState(kDisconnected);
        return 0;
    }

private:
    void setState(State newState) {
        AutoLock lock(mLock);
        mState = newState;
    }

    int mPort = -1;
    int mId = -1;
    mutable Lock mLock;
    State mState = kInit;
};

struct TestAdbGuestAgent : public AdbGuestAgent {
    TestAdbGuestAgent(int id)
        : mId(id), mPrefix(StringFormat("[guest %d] ", id)) {}

    virtual void onHostConnection(ScopedSocket&& socket) override {
        mSocket = std::move(socket);

        uint32_t id = 0;
        ssize_t len = android::base::socketRecv(mSocket.get(), &id, sizeof(id));
        if (len == 0) {
            PLOG(ERROR) << mPrefix << "End of stream";
        } else if (len < 0) {
            PLOG(ERROR) << mPrefix << "I/O Error";
        } else if (len != static_cast<ssize_t>(sizeof(id))) {
            LOG(ERROR) << mPrefix << "Invalid read byte count=" << len;
        }
        mId = static_cast<int>(id);
    }

    int id() const { return mId; }

    void close() { mSocket.close(); }

    int socket() const { return mSocket.get(); }

private:
    int mId = -1;
    ScopedSocket mSocket;
    std::string mPrefix;
};

}  // namespace

TEST(AdbHostListener, reset) {
    AdbHostListener testListener;
    EXPECT_EQ(-1, testListener.port());

    const int adbPort = 0;  // Bind to random port.
    EXPECT_TRUE(testListener.reset(adbPort));
    EXPECT_NE(adbPort, testListener.port());

    // Second reset with the same port number should not fail.
    EXPECT_TRUE(testListener.reset(adbPort));
    EXPECT_NE(adbPort, testListener.port());

    testListener.reset(-1);
    EXPECT_EQ(-1, testListener.port());
}

TEST(AdbHostListener, startListening) {
    TestAdbGuestAgent testAgent(1);
    AdbHostListener testListener;

    testListener.setGuestAgent(&testAgent);
    EXPECT_TRUE(testListener.reset(0));

    {
        TestConnectThread testThread(testListener.port(), 2);

        // Start the thread, it will be unable to send data until
        // startListening()
        // is called. Even if we run the current thread looper for 10 ms.
        testThread.start();
        ThreadLooper::get()->runWithTimeoutMs(10);
        EXPECT_EQ(-1, testAgent.socket());
        EXPECT_EQ(1, testAgent.id());

        // Start listening and run the thread looper again, this time the
        // send should work.
        testListener.startListening();
        ThreadLooper::get()->runWithTimeoutMs(10);
        EXPECT_EQ(2, testAgent.id());
        EXPECT_EQ(TestConnectThread::kDisconnected, testThread.state());
    }

    {
        TestConnectThread testThread(testListener.port(), 3);

        // Start a second thread, it will be unable to send data until
        // startListening() is called again. Even if we run the current
        // thread looper for 10 ms.
        testThread.start();
        ThreadLooper::get()->runWithTimeoutMs(10);
        EXPECT_EQ(2, testAgent.id());

        // Start listening and run the thread looper again, this time the
        // send should work.
        testListener.startListening();
        ThreadLooper::get()->runWithTimeoutMs(10);
        EXPECT_EQ(3, testAgent.id());
        EXPECT_EQ(TestConnectThread::kDisconnected, testThread.state());
    }
}

TEST(AdbHostListener, stopListening) {
    TestAdbGuestAgent testAgent(1);
    AdbHostListener testListener;

    testListener.setGuestAgent(&testAgent);
    EXPECT_TRUE(testListener.reset(0));

    TestConnectThread testThread(testListener.port(), 2);

    // Start the thread, it will be unable to send data until startListening()
    // is called. Even if we run the current thread looper for 10 ms.
    testThread.start();
    ThreadLooper::get()->runWithTimeoutMs(10);
    EXPECT_EQ(-1, testAgent.socket());
    EXPECT_EQ(1, testAgent.id());

    // Start the stop listening, this will prevent the thread to send data
    // even if the looper runs for 10 ms.
    testListener.startListening();
    testListener.stopListening();
    ThreadLooper::get()->runWithTimeoutMs(10);
    EXPECT_EQ(-1, testAgent.socket());
    EXPECT_EQ(1, testAgent.id());
}

TEST(AdbHostListener, notifyServer) {
    TestInputBufferSocketServerThread serverThread;
    int adbClientPort = serverThread.port();
    serverThread.start();

    AdbHostListener testListener(adbClientPort);
    EXPECT_EQ(adbClientPort, testListener.clientPort());

    testListener.reset(0);  // Bind to random port.
    int adbEmulatorPort = testListener.port();

    testListener.notifyServer();

    intptr_t bufferSize = 0;
    EXPECT_TRUE(serverThread.wait(&bufferSize));

    // Verify message content.
    std::string message = StringFormat("host:emulator:%d", adbEmulatorPort);
    std::string expected =
            StringFormat("%04x%s", message.size(), message.c_str());
    EXPECT_EQ(expected.size(), static_cast<size_t>(bufferSize));
    EXPECT_EQ(expected.size(), serverThread.view().size());
    EXPECT_STREQ(expected.c_str(), serverThread.view().c_str());
}

}  // namespace emulation
}  // namespace base
