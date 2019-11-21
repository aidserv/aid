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

#include "android/emulation/AdbHostServer.h"

#include "android/base/StringView.h"
#include "android/base/threads/Thread.h"
#include "android/base/sockets/ScopedSocket.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/base/testing/TestInputBufferSocketServerThread.h"
#include "android/base/testing/TestSystem.h"

#include <gtest/gtest.h>

#include <stdint.h>

namespace android {
namespace emulation {

using android::base::ScopedSocket;
using android::base::StringView;
using android::base::TestSystem;


TEST(AdbHostServer, notify) {
    // Bind to random port to listen to
    android::base::TestInputBufferSocketServerThread serverThread;
    ASSERT_TRUE(serverThread.valid());

    const int emulatorPort = 7648;  // Don't use default (5554) here.
    int clientPort = serverThread.port();
    serverThread.start();

    // Send a message to the server thread.
    EXPECT_TRUE(AdbHostServer::notify(emulatorPort, clientPort));

    intptr_t bufferSize = 0;
    EXPECT_TRUE(serverThread.wait(&bufferSize));

    // Verify message content.
    constexpr StringView kExpected = "0012host:emulator:7648";
    EXPECT_EQ(kExpected.size(), static_cast<size_t>(bufferSize));
    EXPECT_EQ(kExpected.size(), serverThread.view().size());
    EXPECT_STREQ(kExpected.c_str(), serverThread.view().c_str());
}

TEST(AdbHostServer, getClientPortDefault) {
    TestSystem testSystem("/bin", 32);
    const int expected = AdbHostServer::kDefaultAdbClientPort;
    EXPECT_EQ(expected, AdbHostServer::getClientPort());
}

TEST(AdbHostServer, getClientPortWithEnvironmentOverride) {
    TestSystem testSystem("/bin", 32);
    testSystem.envSet("ANDROID_ADB_SERVER_PORT", "1234");
    EXPECT_EQ(1234, AdbHostServer::getClientPort());
}

TEST(AdbHostServer, getClientPortWithInvalidEnvironmentOverride) {
    TestSystem testSystem("/bin", 32);
    testSystem.envSet("ANDROID_ADB_SERVER_PORT", "-1000");
    EXPECT_EQ(-1, AdbHostServer::getClientPort());

    testSystem.envSet("ANDROID_ADB_SERVER_PORT", "65536");
    EXPECT_EQ(-1, AdbHostServer::getClientPort());
}

}  // namespace emulation
}  // namespace android
