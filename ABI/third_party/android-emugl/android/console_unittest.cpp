// Copyright (C) 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/console_internal.h"

#include "android/base/sockets/SocketUtils.h"
#include "android/telephony/modem_driver.h"

#include <gtest/gtest.h>

namespace android {

using namespace base;

namespace console {

#ifdef __APPLE__
// Mac linker has a lot of problems determining that modem_driver.o is
// a dependency, even though we use |android_modem| throughout console.c.
// To make sure we get this dependency, tell the compiler to resolve one of
// the functions defined in modem_driver.c.
static const auto pointer = &android_modem_init;
#endif

// Android studio has a read buffer size of 1024 bytes.
// Older versions of studio use the 'help' command to ping the
// emulator, while newer versions will just use the ping command.
// So for now, we'll be careful to not pass the 1024-byte limit on
// the help printout.
static const int STUDIO_BUFF_SIZE = 1024;

TEST(Console, postauth_help_size_max) {
#ifdef __APPLE__
    // The compiler may still optimize the modem_driver.o dependency out, so
    // force it to locate that symbol.
    ASSERT_TRUE(pointer != nullptr);
#endif

    const int BUFF_SIZE = STUDIO_BUFF_SIZE + 1;
    char buff[BUFF_SIZE];
    int sock[2];

    ASSERT_EQ(0, socketCreatePair(&sock[0], &sock[1]));

    // Create a fake client that will send the output through this socket
    void* opaque = test_control_client_create(sock[1]);
    ASSERT_TRUE(opaque != NULL);

    send_test_string(opaque, "help");

    // Read back the output of 'help'. Should be <= 1024 bytes. If we have
    // received every byte we asked for, then it means we went past the limit.
    memset(buff, 0, BUFF_SIZE);
    bool hasRecvAll = socketRecvAll(sock[0], buff, BUFF_SIZE);
    EXPECT_FALSE(hasRecvAll);

    test_control_client_close(opaque);
    socketClose(sock[1]);
    socketClose(sock[0]);
}

static void ping_test();

TEST(Console, preauth_ping) {
    ping_test();
}

TEST(Console, postauth_ping) {
    ping_test();
}


static void ping_test() {
    // Verify the 'ping' response.
    // This may be used by automated clients that expect
    // an exact response, so we verify the exact response.
    const char EXPECTED_PING_RESPONSE[] = "I am alive!\r\nOK\r\n";
    auto PING_SIZE = strlen(EXPECTED_PING_RESPONSE);
    const int BUFF_SIZE = STUDIO_BUFF_SIZE + 1;
    char buff[BUFF_SIZE];
    int sock[2];

    ASSERT_EQ(0, socketCreatePair(&sock[0], &sock[1]));

    // Create a fake client that will send the output through this socket
    void* opaque = test_control_client_create(sock[1]);
    ASSERT_TRUE(opaque != nullptr);

    send_test_string(opaque, "ping");

    // Read back the output
    memset(buff, 0, BUFF_SIZE);
    bool hasRecvAll = socketRecvAll(sock[0], buff, BUFF_SIZE);

    test_control_client_close(opaque);
    socketClose(sock[1]);
    socketClose(sock[0]);

    // Studio buffer size check
    EXPECT_FALSE(hasRecvAll);
    // Ping size check
    EXPECT_EQ(PING_SIZE, strlen(buff));
    // Ping string comparison check
    EXPECT_EQ(0, strncmp(buff, EXPECTED_PING_RESPONSE, PING_SIZE));
}

}  // namespace console
}  // namespace android
