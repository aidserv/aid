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

#include "android/base/async/AsyncSocketServer.h"

#include "android/base/async/Looper.h"
#include "android/base/async/ThreadLooper.h"
#include "android/base/Log.h"
#include "android/base/sockets/ScopedSocket.h"
#include "android/base/threads/Async.h"

#include <gtest/gtest.h>

#include <memory>
#include <functional>

namespace android {
namespace base {

namespace {

struct TestData {
    TestData(Looper* looper, int port) : mLooper(looper), mPort(port) {}

    // Start a server that will call ::onConnection on each connection attempt.
    bool startServer(AsyncSocketServer::LoopbackMode mode) {
        auto callback =
                std::bind(&TestData::onConnection, this, std::placeholders::_1);
        mServer = AsyncSocketServer::createTcpLoopbackServer(mPort, callback,
                                                             mode, mLooper);
        if (!mServer.get()) {
            return false;
        }
        mPort = mServer->port();
        mServer->startListening();
        return true;
    }

    // On every connection, increment the counter, read one byte from the
    // socket, then close it! Note that this doesn't start listening for
    // new connections, this is left to the test itself.
    bool onConnection(int socket) {
        this->mCounter++;
        char c = 0;
        EXPECT_EQ(1, socketRecv(socket, &c, 1));
        EXPECT_EQ(0x42, c);
        socketClose(socket);
        return true;
    }

    // Connect to IPv4 loopback |port| and send one byte of data.
    static intptr_t connectAndWriteTo4(int port) {
        // Blocking connect to the socket.
        int s = socketTcp4LoopbackClient(port);
        if (s < 0) {
            PLOG(ERROR) << "Connection failed: ";
            return -1;
        }
        // Write a byte to it.
        char c = 0x42;
        int ret = socketSend(s, &c, 1);
        if (ret < 0) {
            PLOG(ERROR) << "Could not send data";
        }
        socketClose(s);
        return (ret == 1) ? 0 : -1;
    }

    // Connect to IPv6 loopback |port| and send one byte of data.
    static intptr_t connectAndWriteTo6(int port) {
        // Blocking connect to the socket.
        int s = socketTcp6LoopbackClient(port);
        if (s < 0) {
            return -1;
        }
        // Write a byte to it.
        char c = 0x42;
        int ret = socketSend(s, &c, 1);
        socketClose(s);
        return (ret == 1) ? 0 : -1;
    }

    Looper* mLooper = nullptr;
    int mPort = 0;
    int mCounter = 0;
    std::unique_ptr<AsyncSocketServer> mServer;
};

}  // namespace

TEST(AsyncSocketServer, createTcpLoopbackServerIPv4) {
    int kPort = 0;  // random high value?
    int kDelayMs = 20;
    Looper* looper = ThreadLooper::get();

    TestData data(looper, kPort);
    ASSERT_TRUE(data.startServer(AsyncSocketServer::kIPv4));
    EXPECT_EQ(AsyncSocketServer::kIPv4, data.mServer->getListenMode());
    EXPECT_EQ(0, data.mCounter);

    // Start two background threads that connect to the port.
    // Only the first one will succeed because the AsyncSocketServer
    // will stop listening automatically after the first connection.
    auto connector = std::bind(&TestData::connectAndWriteTo4, data.mPort);
    android::base::async(connector);
    android::base::async(connector);

    int ret = looper->runWithDeadlineMs(looper->nowMs() + kDelayMs);
    EXPECT_EQ(1, data.mCounter);
    EXPECT_EQ(ETIMEDOUT, ret);

    // Start listening, which should unblock the second background thread.
    data.mServer->startListening();
    ret = looper->runWithDeadlineMs(looper->nowMs() + kDelayMs);
    EXPECT_EQ(2, data.mCounter);
    EXPECT_EQ(ETIMEDOUT, ret);
}

TEST(AsyncSocketServer, createTcpLoopbackServerIPv6) {
    int kPort = 0;  // random high value?
    int kDelayMs = 20;
    Looper* looper = ThreadLooper::get();

    TestData data(looper, kPort);
    ASSERT_TRUE(data.startServer(AsyncSocketServer::kIPv6));
    EXPECT_EQ(AsyncSocketServer::kIPv6, data.mServer->getListenMode());
    EXPECT_EQ(0, data.mCounter);

    // Start two background threads that connect to the port.
    // Only the first one will succeed because the AsyncSocketServer
    // will stop listening automatically after the first connection.
    auto connector = std::bind(&TestData::connectAndWriteTo6, data.mPort);
    android::base::async(connector);
    android::base::async(connector);

    int ret = looper->runWithDeadlineMs(looper->nowMs() + kDelayMs);
    EXPECT_EQ(1, data.mCounter);
    EXPECT_EQ(ETIMEDOUT, ret);

    // Start listening, which should unblock the second background thread.
    data.mServer->startListening();
    ret = looper->runWithDeadlineMs(looper->nowMs() + kDelayMs);
    EXPECT_EQ(2, data.mCounter);
    EXPECT_EQ(ETIMEDOUT, ret);
}

TEST(AsyncSocketServer, createTcpLoopbackServerIPv4AndIPv6) {
    int kPort = 0;  // random high value?
    int kDelayMs = 20;
    Looper* looper = ThreadLooper::get();

    TestData data(looper, kPort);
    ASSERT_TRUE(data.startServer(AsyncSocketServer::kIPv4AndIPv6));
    EXPECT_EQ(AsyncSocketServer::kIPv4AndIPv6, data.mServer->getListenMode());
    EXPECT_EQ(0, data.mCounter);

    // Start two background threads that connect to the port.
    // Only the first one will succeed because the AsyncSocketServer
    // will stop listening automatically after the first connection.
    // NOTE: The first one connects through IPv4, the second one through IPv6.
    auto connector4 = std::bind(&TestData::connectAndWriteTo4, data.mPort);
    android::base::async(connector4);

    auto connector6 = std::bind(&TestData::connectAndWriteTo6, data.mPort);
    android::base::async(connector6);

    int ret = looper->runWithDeadlineMs(looper->nowMs() + kDelayMs);
    EXPECT_EQ(1, data.mCounter);
    EXPECT_EQ(ETIMEDOUT, ret);

    // Start listening, which should unblock the second background thread.
    data.mServer->startListening();
    ret = looper->runWithDeadlineMs(looper->nowMs() + kDelayMs);
    EXPECT_EQ(2, data.mCounter);
    EXPECT_EQ(ETIMEDOUT, ret);
}

}  // namespace base
}  // namespace android
