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

#include "android/emulation/AdbGuestPipe.h"

#include "android/base/async/ThreadLooper.h"
#include "android/base/Log.h"
#include "android/base/sockets/ScopedSocket.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/base/StringFormat.h"
#include "android/base/threads/Thread.h"
#include "android/emulation/testing/TestAndroidPipeDevice.h"

#include <gtest/gtest.h>

// Because gtest includes Windows headers that declare the ERROR
// macro that prevents LOG(ERROR) from compiling!!
#ifdef _WIN32
#undef ERROR
#endif

#include <memory>
#include <vector>

namespace android {
namespace emulation {

using android::base::ScopedSocket;
using android::base::StringFormat;
using android::base::StringView;

using TestGuest = TestAndroidPipeDevice::Guest;

namespace {

// A Mock AdbHostAgent that will be used during testing. It doesn't depend
// on any TCP ports.
class MockAdbHostAgent : public AdbHostAgent {
public:
    MockAdbHostAgent() {
        auto service = new AdbGuestPipe::Service(this);
        mGuestAgent = service;
        AndroidPipe::Service::add(service);
    }

    ~MockAdbHostAgent() {
        if (mThread.get()) {
            mThread->wait(nullptr);
        }
        AndroidPipe::Service::resetAll();
    }

    // AdbHostAgent overrides.
    void setAgent(AdbGuestAgent* guestAgent) { mGuestAgent = guestAgent; }

    virtual void startListening() override { mListening = true; }
    virtual void stopListening() override { mListening = false; }
    virtual void notifyServer() override { mServerNotificationCount++; }

    // Accessors.
    virtual bool isListening() const { return mListening; }

    virtual int serverNotificationCount() const {
        return mServerNotificationCount;
    }

    // Create a socket pair and a thread that will push |data| into it
    // before trying to read a single byte from one end of the pair.
    // The other end is passed to a new active guest.
    void createFakeConnection(StringView data) {
        CHECK(mListening);
        if (mThread.get()) {
            mThread->wait(nullptr);
        }
        mThread.reset(new ConnectorThread(data));
        mListening = false;
        mGuestAgent->onHostConnection(mThread->releaseOutSocket());
        mThread->start();
    }

private:
    // A small thread that will connect to a given port and send some
    // data through the socket. It will then try to read a single byte
    // before exiting.
    class ConnectorThread : public android::base::Thread {
    public:
        ConnectorThread(StringView data) : Thread(), mData(data) {
            int inSocket, outSocket;
            if (android::base::socketCreatePair(&inSocket, &outSocket) < 0) {
                PLOG(ERROR) << "Could not create socket pair";
                return;
            }
            // Make the sockets blocking for this test to work.
            android::base::socketSetBlocking(inSocket);
            android::base::socketSetBlocking(outSocket);

            mInSocket.reset(inSocket);
            mOutSocket.reset(outSocket);
        }

        bool valid() const { return mInSocket.valid() && mOutSocket.valid(); }

        int releaseOutSocket() { return mOutSocket.release(); }

        virtual intptr_t main() override {
            if (mData.size() > 0) {
                if (!android::base::socketSendAll(
                            mInSocket.get(), mData.c_str(), mData.size())) {
                    DPLOG(ERROR) << "I/O error when sending data";
                    return -1;
                }
            }
            char buf[1] = {};
            ssize_t len = android::base::socketRecv(mInSocket.get(), buf,
                                                    sizeof(buf));
            if (len < 0) {
                DPLOG(ERROR) << "I/O error when receiving data";
            } else if (len == 0) {
                DLOG(ERROR) << "Disconnected";
            }
            mInSocket.close();
            return 0;
        }

    private:
        ScopedSocket mInSocket;
        ScopedSocket mOutSocket;
        std::string mData;
    };

    AdbGuestAgent* mGuestAgent = nullptr;
    bool mListening = false;
    int mServerNotificationCount = 0;
    std::unique_ptr<ConnectorThread> mThread;
};

// The socket associated with |guest| is in non-blocking mode, which
// means that read() attempts will sometimes return PIPE_ERROR_AGAIN.
// This function handles these by waiting a little, giving some time
// slices for the ConnectorThread to write properly.
int blockingRead(TestGuest* guest, void* buffer, size_t size) {
    int ret = guest->read(buffer, size);
    if (ret != PIPE_ERROR_AGAIN) {
        return ret;
    }
    android::base::ThreadLooper::get()->runWithTimeoutMs(2);
    return guest->read(buffer, size);
}

}  // namespace

TEST(AdbGuestPipe, createService) {
    AndroidPipe::Service::resetAll();

    // Create a host agent and a new service and register it.
    MockAdbHostAgent adbHost;
}

TEST(AdbGuestPipe, createOneGuest) {
    // NOTE: This does Service::resetAll() on creation and destruction for us.
    TestAndroidPipeDevice testDevice;

    MockAdbHostAgent adbHost;

    // Create a new guest connection from the test device.
    auto guest = TestGuest::create();
    EXPECT_TRUE(guest);
    EXPECT_EQ(0, guest->connect("qemud:adb"));
    EXPECT_EQ(6, guest->write("accept", 6));

    static constexpr StringView kMessage = "Hello World!";
    adbHost.createFakeConnection(kMessage);

    char reply[3] = {};
    EXPECT_EQ(2, guest->read(reply, 2));
    EXPECT_STREQ("ok", reply);
    EXPECT_EQ(5, guest->write("start", 5));

    char buffer[kMessage.size() + 1] = {};
    const ssize_t expectedSize = static_cast<ssize_t>(kMessage.size());
    EXPECT_EQ(expectedSize, blockingRead(guest, buffer, kMessage.size()));
    EXPECT_STREQ(kMessage.c_str(), buffer);

    EXPECT_EQ(1, guest->write("x", 1));
}

TEST(AdbGuestPipe, createGuestWithBadAcceptCommand) {
    TestAndroidPipeDevice testDevice;

    MockAdbHostAgent adbHost;

    // Create a new guest connection from the test device.
    auto guest = TestGuest::create();
    EXPECT_TRUE(guest);
    EXPECT_EQ(0, guest->connect("qemud:adb"));

    // This write should fail with PIPE_ERROR_IO because the other end
    // is waiting for an 'accept'.
    EXPECT_EQ(PIPE_ERROR_IO, guest->write("ACCEPT", 6));
}

TEST(AdbGuestPipe, createGuestWithCloseOnAccept) {
    TestAndroidPipeDevice testDevice;

    MockAdbHostAgent adbHost;

    // Create a new guest connection from the test device.
    auto guest = TestGuest::create();
    EXPECT_TRUE(guest);
    EXPECT_EQ(0, guest->connect("qemud:adb"));

    // Force-close the connection.
    guest->close();
}

TEST(AdbGuestPipe, createGuestWithCloseBeforeConnection) {
    TestAndroidPipeDevice testDevice;

    MockAdbHostAgent adbHost;

    // Create a guest, and verify that this invokes startListening on the
    // guest agent.
    std::unique_ptr<TestGuest> guest(TestGuest::create());
    EXPECT_EQ(0, guest->connect("qemud:adb"));
    EXPECT_FALSE(adbHost.isListening());
    EXPECT_EQ(6, guest->write("accept", 6));
    EXPECT_TRUE(adbHost.isListening());

    guest->close();
    EXPECT_FALSE(adbHost.isListening());
}

TEST(AdbGuestPipe, createGuestWithCloseOnReply) {
    TestAndroidPipeDevice testDevice;

    MockAdbHostAgent adbHost;
    AndroidPipe::Service::add(new AdbGuestPipe::Service(&adbHost));

    // Create a new guest connection from the test device.
    auto guest = TestGuest::create();
    EXPECT_TRUE(guest);
    EXPECT_EQ(0, guest->connect("qemud:adb"));

    EXPECT_EQ(6, guest->write("accept", 6));

    static constexpr StringView kMessage = "Hello World!";
    adbHost.createFakeConnection(kMessage);

    // Force-close the connection.
    guest->close();
}

TEST(AdbGuestPipe, createGuestWithBadStartCommand) {
    TestAndroidPipeDevice testDevice;

    MockAdbHostAgent adbHost;

    // Create a new guest connection from the test device.
    auto guest = TestGuest::create();
    EXPECT_TRUE(guest);
    EXPECT_EQ(0, guest->connect("qemud:adb"));

    EXPECT_EQ(6, guest->write("accept", 6));

    static constexpr StringView kMessage = "Hello World!";
    adbHost.createFakeConnection(kMessage);

    char reply[3] = {};
    EXPECT_EQ(2, guest->read(reply, 2));
    EXPECT_STREQ("ok", reply);

    // This write should fail with PIPE_ERROR_IO because the other end
    // is waiting for a 'start'. NOTE: This will result in a broken pipe
    // error for the connector thread waiting for a reply.
    EXPECT_EQ(PIPE_ERROR_IO, guest->write("START", 5));
}

TEST(AdbGuestPipe, createGuestWithCloseOnStart) {
    TestAndroidPipeDevice testDevice;

    MockAdbHostAgent adbHost;

    // Create a new guest connection from the test device.
    auto guest = TestGuest::create();
    EXPECT_TRUE(guest);
    EXPECT_EQ(0, guest->connect("qemud:adb"));

    EXPECT_EQ(6, guest->write("accept", 6));

    static constexpr StringView kMessage = "Hello World!";
    adbHost.createFakeConnection(kMessage);

    char reply[3] = {};
    EXPECT_EQ(2, guest->read(reply, 2));
    EXPECT_STREQ("ok", reply);

    // Force-close the connection.
    guest->close();
}

TEST(AdbGuestPipe, createGuestWhichClosesTheConnection) {
    // NOTE: This does Service::resetAll() on creation and destruction for us.
    TestAndroidPipeDevice testDevice;

    MockAdbHostAgent adbHost;

    // Create a new guest connection from the test device.
    auto guest = TestGuest::create();
    EXPECT_TRUE(guest);
    EXPECT_EQ(0, guest->connect("qemud:adb"));
    EXPECT_EQ(6, guest->write("accept", 6));

    static constexpr StringView kMessage = "Hello World!";
    adbHost.createFakeConnection(kMessage);

    char reply[3] = {};
    EXPECT_EQ(2, guest->read(reply, 2));
    EXPECT_STREQ("ok", reply);
    EXPECT_EQ(5, guest->write("start", 5));

    // Read only partial bytes from the connection.
    char buffer[kMessage.size() + 1] = {};
    const ssize_t expectedSize = static_cast<ssize_t>(kMessage.size()) / 2;
    EXPECT_EQ(expectedSize, blockingRead(guest, buffer, expectedSize));

    // Force-close the connection now.
    guest->close();
}

TEST(AdbGuestPipe, createMultipleGuestConnections) {
    TestAndroidPipeDevice testDevice;

    MockAdbHostAgent adbHost;

    const int kCount = 8;

    // Create kCount guests that all connect at the same time.
    std::unique_ptr<TestGuest> guests[kCount];
    for (int n = 0; n < kCount; ++n) {
        guests[n].reset(TestGuest::create());
        EXPECT_TRUE(guests[n].get());
        EXPECT_EQ(0, guests[n]->connect("qemud:adb"));
        EXPECT_FALSE(adbHost.isListening());
    }

    // Create kCount threads that will send data to each guest in succession.
    for (int n = 0; n < kCount; ++n) {
        auto guest = guests[n].get();
        EXPECT_EQ(6, guest->write("accept", 6));
        EXPECT_TRUE(adbHost.isListening());

        std::string message = StringFormat("Hello %d", n + 1);
        adbHost.createFakeConnection(message);

        char reply[3] = {};
        EXPECT_EQ(2, guest->read(reply, 2)) << n + 1;
        EXPECT_STREQ("ok", reply) << n + 1;
        EXPECT_EQ(5, guest->write("start", 5)) << n + 1;
        EXPECT_FALSE(adbHost.isListening());

        std::string buffer;
        buffer.resize(message.size());
        EXPECT_EQ(static_cast<ssize_t>(message.size()),
                  blockingRead(guest, &buffer[0], message.size()))
                << n + 1;

        EXPECT_EQ(message, buffer);
        EXPECT_EQ(1, guest->write("x", 1)) << n + 1;
        guest->close();
    }

    EXPECT_FALSE(adbHost.isListening());
}

}  // namespace emulation
}  // namespace android
