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

#include "android/base/async/Looper.h"
#include "android/base/Log.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/base/sockets/ScopedSocket.h"
#include "android/wear-agent/PairUpWearPhone.h"
#include "android/wear-agent/testing/WearAgentTestUtils.h"

#include <gtest/gtest.h>

#include <stdio.h>
#include <sys/unistd.h>

#define DEBUG_TEST 0

#if DEBUG_TEST
#define DPRINT(...)  fprintf(stderr, __VA_ARGS__)
#else
#define DPRINT(...)  ((void)0)
#endif

namespace android {
namespace wear {

using namespace ::android::base;
using namespace ::android::wear::testing;

// This doesn't run on Windows because the test currently relies on
// fork() to run the pairing agent in a child process.
#ifndef _WIN32

static void runPairUp(const std::vector<std::string>& deviceList,
                      int adbHostPort) {
    Looper* looper = Looper::create();
    {
        PairUpWearPhone pairUp(looper, deviceList, adbHostPort);
        looper->run();
    }
    delete looper;
}

static bool testWrapper(bool usbPhone) {
    int adbMockServerPort = -1;
    ScopedSocket adbMockServerSocket(
            testStartMockServer(&adbMockServerPort));
    if (!adbMockServerSocket.valid()) {
        DPRINT("ERROR: cannot open adb server port: %d\n",
               adbMockServerPort);
        return false;
    }
    int consoleServerPort = -1;
    ScopedSocket consoleServerSocket;
    char phoneDevice[128] = {'\0'};
    const char kUsbPhoneDevice[] = "070fe93a0b2c1fdb";
    if (usbPhone) {
        snprintf(phoneDevice, sizeof(phoneDevice), kUsbPhoneDevice);
    } else {
        consoleServerSocket.reset(testStartMockServer(&consoleServerPort));
        if (!consoleServerSocket.valid()) {
            return false;
        }
        snprintf(phoneDevice,
                 sizeof(phoneDevice),
                 "emulator-%d",
                 consoleServerPort);
    }
    const char kWearDevice[] = "emulator-6558";

    std::vector<std::string> deviceList;
    deviceList.push_back(kWearDevice);
    deviceList.push_back(phoneDevice);
    int childpid = fork();
    if (childpid == 0) {
        // Child process runs the pairing agent.
        runPairUp(deviceList, adbMockServerPort);
        _exit(0);
    } else if (childpid < 0) {
        DPRINT("Could not fork!\n");
        return false;
    }
    // In the parent process.
    bool status = testRunMockAdbServer(adbMockServerSocket.get(),
                                       consoleServerSocket.get(),
                                       kWearDevice,
                                       phoneDevice,
                                       usbPhone);
    return status;
}

TEST(PairUpWearPhone, TestUsbPhone) {
    EXPECT_TRUE(testWrapper(true));
}

TEST(PairUpWearPhone, TestEmulatorPhone) {
    EXPECT_TRUE(testWrapper(false));
}

#endif  // !_WIN32

} // namespace wear
} // namespace android
