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
#include "android/base/sockets/ScopedSocket.h"
#include "android/base/sockets/SocketDrainer.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/wear-agent/testing/WearAgentTestUtils.h"
#include "android/wear-agent/WearAgent.h"

#include <gtest/gtest.h>

// Set to 1 to enable debugging traces.
#define DEBUG_TEST 0

#if DEBUG_TEST
#define  DPRINT(...)  fprintf(stderr, __VA_ARGS__)
#else
#define DPRINT(...)  ((void)0)
#endif


namespace android {
namespace wear {

using namespace ::android::base;
using namespace ::android::wear::testing;

static volatile bool stopped = false;

static void on_time_up (void* opaque, Looper::Timer* timer) {
    Looper* looper = static_cast<Looper*>(opaque);
    looper->forceQuit();
    stopped = true;
}

// Simple test to run agent for 1 mil second and exit without crash
TEST(WearAgent, SimpleTest) {
    int adbHostPort = 5038; // intentionally different from adb defaut port 5037
    int serverSocket = testStartMockServer(&adbHostPort);
    ASSERT_GE(serverSocket, 0);
    int milSecondsToRun = 1;
    Looper* mainLooper = Looper::create();
    {
        stopped = false;

        android::wear::WearAgent agent(mainLooper, adbHostPort);
        SocketDrainer drainer(mainLooper);

        Looper::Timer* timer = mainLooper->createTimer(on_time_up, mainLooper);
        const Looper::Duration dl = milSecondsToRun;
        timer->startRelative(dl);

        while (!stopped) {
            mainLooper->run();
        }
        delete timer;
    }
    delete mainLooper;
}

#ifndef _WIN32

static bool testTrackDevicesQuery(int socketFd, ScopedSocket* agentSocket) {
    agentSocket->reset(socketAcceptAny(socketFd));
    if (!agentSocket->valid()) {
        DPRINT("error happens: %d < 0\n", agentSocket);
        return false;
    }
    return testExpectMessageFromSocket(agentSocket->get(),
                                       "host:track-devices");
}

static void runWearAgent(int adbHostPort, int milSecondsToRun) {
    Looper* mainLooper = Looper::create();
    {
        stopped = false;

        android::wear::WearAgent agent(mainLooper, adbHostPort);
        SocketDrainer drainer(mainLooper);

        Looper::Timer* timer = NULL;
        if (milSecondsToRun > 0) {
            timer = mainLooper->createTimer(on_time_up, mainLooper);
            const Looper::Duration dl = milSecondsToRun;
            timer->startRelative(dl);
        }

        while (!stopped) {
            mainLooper->run();
        }

        if (milSecondsToRun > 0) {
            delete timer;
        }
    }

    DPRINT("wear agent exits\n");
    fflush(stdout);
    delete mainLooper;
}

// Test that the agent should properly setup ip forwarding when there
// are one wear and one phone

static bool testWearAgent(bool usbPhone) {
    //VERBOSE_DISABLE(adb);
    int adbHostPort = -1; // different from adb defaut port 5037
    ScopedSocket adbServerSocket(testStartMockServer(&adbHostPort));
    if (!adbServerSocket.valid()) {
        DPRINT("ERROR: cannot open adb server port: %d\n", adbHostPort);
        return false;
    }
    int milSecondsToRun = 100;
    int childpid = fork();
    if (childpid == 0) {
        runWearAgent(adbHostPort, milSecondsToRun);
        _exit(0);
    } else {
        if (childpid < 0) {
            DPRINT("Cannot fork!\n");
            return false;
        }
    }
    ScopedSocket agentSocketTracking;
    if (!testTrackDevicesQuery(adbServerSocket.get(), &agentSocketTracking) ||
        !testSendToSocket(agentSocketTracking.get(), "OKAY")) {
        return false;
    }

    ScopedSocket consoleSocket;

    char phoneDevice[128]={'\0'};
    if (usbPhone) {
        snprintf(phoneDevice, sizeof(phoneDevice), "070fe93a0b2c1fdb");
    } else {
        // start a new server at port 6556
        int consolePort = 6556;
        consoleSocket.reset(testStartMockServer(&consolePort));
        if (!consoleSocket.valid()) {
            return false;
        }
        snprintf(phoneDevice, sizeof(phoneDevice), "emulator-%d", consolePort);
    }
    const char kWearDevice[] = "emulator-6554";
    char kDevices[1024] = {'\0'};
    snprintf(kDevices,
             sizeof(kDevices),
             "%s device %s device",
             kWearDevice,
             phoneDevice);
    char buf[1024] = {'\0'};
    snprintf(buf, sizeof(buf), "%04x%s", (int)(strlen(kDevices)), kDevices);
    return testSendToSocket(agentSocketTracking.get(), buf) &&
           testRunMockAdbServer(adbServerSocket.get(),
                                consoleSocket.get(),
                                kWearDevice,
                                phoneDevice,
                                usbPhone);
}

TEST(WearAgent, PairUpWearToUsbPhone) {
    EXPECT_TRUE(testWearAgent(true));
}

TEST(WearAgent, PairUpWearToEmulatorPhone) {
    EXPECT_TRUE(testWearAgent(false));
}
#endif

} // namespace wear
} // namespace android
