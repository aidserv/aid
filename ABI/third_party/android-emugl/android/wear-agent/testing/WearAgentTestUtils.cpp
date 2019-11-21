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

#include "android/wear-agent/testing/WearAgentTestUtils.h"

#include "android/base/sockets/ScopedSocket.h"
#include "android/base/sockets/SocketErrors.h"
#include "android/base/sockets/SocketUtils.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#define DEBUG_TEST 0

#if DEBUG_TEST
#define DPRINT(...)  fprintf(stderr, __VA_ARGS__)
#else
#define DPRINT(...)  ((void)0)
#endif

namespace android {
namespace wear {
namespace testing {

using namespace android::base;

bool testSendToSocket(int socketFd, const char* message) {
    DPRINT("---sending message: [%s]\n", message);
    int size = strlen(message);
    const char* ptr = message;
    while (size > 0) {
        ssize_t ret = socketSend(socketFd, ptr, size);
        if (ret < 0 && (errno == EWOULDBLOCK || errno == EAGAIN)) {
            continue;
        } else if (ret <= 0) {
            DPRINT("---failure [%s]\n", message);
            return -1;
        } else {
            ptr += ret;
            size -= ret;
        }
    }
    DPRINT("---success [%s]\n", message);
    return true;
}

bool testReceiveMessageFromSocket(int socketFd, char* ptr, int size) {
    while (size > 0) {
        int ret = socketRecv(socketFd, ptr, size);
        if (ret < 0 && (errno == EWOULDBLOCK || errno == EAGAIN)) {
            continue;
        } else if (ret <= 0) {
            return false;
        } else {
            ptr += ret;
            size -= ret;
        }
    }
    return true;
}

bool testExpectMessageFromSocket(int socketFd,
                                 const char* message,
                                 int header) {
    int size=0;
    if (header) {
        char buf[1024] = {'\0'};
        if (!testReceiveMessageFromSocket(socketFd, buf, 4)) {
            DPRINT("---cannot receive 4 bytes\n");
            fflush(stdout);
            return false;
        }
        size = 0;
        buf[4] = '\0';
        if (1 != sscanf(buf, "%x", &size) || size <= 0) {
            DPRINT("---cannot receive message length\n");
            fflush(stdout);
            return false;
        }
    } else {
        size = strlen(message);
    }
    char buf2[1024] = {'\0'};
    bool status = testReceiveMessageFromSocket(socketFd, buf2, size);
    DPRINT("===received message: [%.*s]\n", size, buf2);
    fflush(stdout);
    if (!status) {
        return status;
    }
    if (0 == strncmp(message, buf2, size)) {
        return true;
    } else {
        DPRINT("===MISMATCH [%.*s] expected [%.*s]\n", size, buf2, size, message);
        return false;
    }
}

bool testRunMockAdbServer(int adbServerSocket,
                          int consoleServerSocket,
                          const char* wearDevice,
                          const char* phoneDevice,
                          bool usbPhone) {
    char buf[1024] = {'\0'};
    const char kProperty[] = "shell:getprop ro.product.name";

    // query regarding the watch
    {
        ScopedSocket s(socketAcceptAny(adbServerSocket));
        if (!s.valid()) {
            DPRINT("Could not accept connection!?\n");
            return false;
        }

        snprintf(buf, sizeof buf, "host:transport:%s", wearDevice);

        if (!testExpectMessageFromSocket(s.get(), buf) ||
            !testSendToSocket(s.get(), "OKAY") ||
            !testExpectMessageFromSocket(s.get(), kProperty) ||
            !testSendToSocket(s.get(), "OKAYclockwork")) {
            return false;
        }
    }

    // query regarding the phone
    {
        ScopedSocket s(socketAcceptAny(adbServerSocket));
        if (!s.valid()) {
            return false;
        }

        snprintf(buf, sizeof(buf), "host:transport:%s", phoneDevice);
        if (!testExpectMessageFromSocket(s.get(), buf) ||
            !testSendToSocket(s.get(), "OKAY") ||
            !testExpectMessageFromSocket(s.get(), kProperty) ||
            !testSendToSocket(s.get(), "OKAYsdk")) {
            return false;
        }
    }

    const char kPackages[] =
            "shell:pm list packages com.google.android.wearable";

    {
        ScopedSocket s(socketAcceptAny(adbServerSocket));
        if (!s.valid()) {
            return false;
        }

        snprintf(buf, sizeof(buf), "host:transport:%s", phoneDevice);
        if (!testExpectMessageFromSocket(s.get(), buf) ||
            !testSendToSocket(s.get(), "OKAY") ||
            !testExpectMessageFromSocket(s.get(), kPackages) ||
            !testSendToSocket(s.get(), "com.google.android.wearable.app")) {
            return false;
        }
    }

    // communication between phone-console and agent
    if (usbPhone) {
        ScopedSocket s(socketAcceptAny(adbServerSocket));
        if (!s.valid()) {
            DPRINT("ZZZ");
            return false;
        }
        char buf[1024];
        snprintf(buf,
                 sizeof(buf),
                 "host-serial:%s:forward:tcp:5601;tcp:5601",
                 phoneDevice);
        if (!testExpectMessageFromSocket(s.get(), buf) ||
            !testSendToSocket(s.get(), "OKAY")) {
            DPRINT("YYY");
            return false;
        }
    } else {
        ScopedSocket s(socketAcceptAny(consoleServerSocket));
        if (!s.valid()) {
            DPRINT("ERROR: cannot accept console port\n");
            return false;
        }
        if (!testSendToSocket(s.get(), "Android Console\nOK\n") ||
            !testExpectMessageFromSocket(s.get(),
                "redir add tcp:5601:5601\nquit\n", 0)) {
            DPRINT("XXX");
            return false;
        }
    }

    return true;
}

int testStartMockServer(int* hostPort) {
    int socket = socketTcp4LoopbackServer(0);
    if (socket < 0) {
        DPRINT("Could not find available TCP port for test!\n");
        return -1;
    }
    // Get socket port now.
    *hostPort = socketGetPort(socket);
    DPRINT("Using TCP loopback port %d\n", *hostPort);
    return socket;
}

}  // namespace testing
}  // namespace wear
}  // namespace android
