/* Copyright (C) 2016 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

#include "android/telephony/modem.h"
#include "android/base/ArraySize.h"

#include <gtest/gtest.h>

#include <stdio.h>
#include <string.h>

static const int kBasePort = 42;

class ModemTest : public ::testing::Test {
protected:
    void SetUp() override {
        mModem = amodem_create(kBasePort,
                               &ModemTest::onUnsolicitedCommand,
                               this);
    }

    void TearDown() override {
        if (mModem) {
            amodem_destroy(mModem);
            mModem = nullptr;
        }
    }

    static void onUnsolicitedCommand(void* opaque, const char* message) {
    }

    int openLogicalChannel(const char* name) {
        // Create and send command
        char command[1024];
        snprintf(command, sizeof(command), "AT+CCHO=%s", name);
        command[sizeof(command) - 1] = '\0';
        const char* reply = amodem_send(mModem, command);
        if (reply == nullptr) {
            return -1;
        }

        // Parse the reply
        int channel;
        char ok[4] = { 0 };
        char dummy;
        if (sscanf(reply, "%d%3c%c", &channel, ok, &dummy) != 2) {
            return -2;
        }
        if (strcmp(ok, "\rOK") != 0) {
            return -3;
        }
        return channel;
    }

    bool closeLogicalChannel(int channel) {
        char command[128];
        snprintf(command, sizeof(command), "AT+CCHC=%d", channel);
        command[sizeof(command) - 1] = '\0';
        const char* reply = amodem_send(mModem, command);
        return strcmp("+CCHC\rOK", reply) == 0;
    }

    AModem mModem;
};

TEST_F(ModemTest, OpenLogicalChannel) {
    // Open a lgoical channel (CCHO operation) with some random name
    int firstChannel = openLogicalChannel("F6238748383");

    // Channels must start at 1 or above
    ASSERT_GE(firstChannel, 1);

    int secondChannel = openLogicalChannel("F6238748383");
    ASSERT_GE(secondChannel, 1);

    // Make sure we got a different channel
    ASSERT_NE(firstChannel, secondChannel);

    closeLogicalChannel(firstChannel);
    closeLogicalChannel(secondChannel);
}

TEST_F(ModemTest, CloseLogicalChannel) {
    int channel = openLogicalChannel("F6238748383");
    ASSERT_GE(channel, 1);

    // Should not be able to close another channel
    ASSERT_FALSE(closeLogicalChannel(channel + 1));

    // Should be able to close channel but only once
    ASSERT_TRUE(closeLogicalChannel(channel));
    ASSERT_FALSE(closeLogicalChannel(channel));
}

static const char* kValidReply = "+CGLA: 144,0,FF403eE23cE12eC11461ed377e85d386a8dfee6b864bd85b0bfaa5af81CA16616e64726f69642e636172726965726170692e637473E30aDB080000000000000000\rOK";
static const char* kCmeError = "+CME ERROR: ";

TEST_F(ModemTest, TransmitLogicalChannel) {
    static const struct {
        const char* ChannelName;
        const char* CommandFormat;
        const char* ExpectedReply;
        bool FullMatch;
    } kData[] = {
        // No open channel
        { nullptr, "AT+CGLA=1,10,80CAFF4000", kCmeError, false },
        // Open channel, correct format, correct reply, lower 4 bits in class
        // shouldn't matter
        { "A00000015141434C00", "AT+CGLA=%d,10,80CAFF4000", kValidReply, true },
        { "A00000015141434C00", "AT+CGLA=%d,10,8FCAFF4000", kValidReply, true },
        { "A00000015141434C00", "AT+CGLA=%d,10,87CAFF4000", kValidReply, true },
        { "A00000015141434C00", "AT+CGLA=%d,10,83CAFF4000", kValidReply, true },
        // Correct data but channel name with no matching data
        { "ABCDEF", "AT+CGLA=%d,10,80CAFF4000", kCmeError, false },
        // Mismatching length and command size, various combinations
        { "F6238748383", "AT+CGLA=%d,9,80CAFF4000", kCmeError, false },
        { "F6238748383", "AT+CGLA=%d,11,80CAFF4000", kCmeError, false },
        { "F6238748383", "AT+CGLA=%d,10,80CAFF40", kCmeError, false },
        // Too short, must be at least 10 characters to be an APDU command
        { "F6238748383", "AT+CGLA=%d,8,80CAFF40", kCmeError, false },
        // Open a channel but use an incorrect channel number
        { "F6238748383", "AT+CGLA=10000,10,80CAFF4000", kCmeError, false },
        { "F6238748383", "AT+CGLA=0,10,80CAFF4000", kCmeError, false },
        { "F6238748383", "AT+CGLA=-1,10,80CAFF4000", kCmeError, false },
        // Attempt to use an incorrect class or instruction (first two bytes)
        { "F6238748383", "AT+CGLA=%d,10,90CAFF4000", kCmeError, false },
        { "F6238748383", "AT+CGLA=%d,10,80CBFF4000", kCmeError, false },
        { "F6238748383", "AT+CGLA=%d,10,A0DBFF4000", kCmeError, false },
    };
    static const size_t kNumTests = android::base::arraySize(kData);

    for (size_t i = 0; i < kNumTests; ++i) {
        int channel = -1;
        if (kData[i].ChannelName) {
            channel = openLogicalChannel(kData[i].ChannelName);
            ASSERT_GE(channel, 1);
        }
        char command[1024];
        if (strstr(kData[i].CommandFormat, "%d") != nullptr) {
            // Insert channel number if there's a format argument in there
            snprintf(command, sizeof(command), kData[i].CommandFormat, channel);
        } else {
            snprintf(command, sizeof(command), "%s", kData[i].CommandFormat);
        }
        command[sizeof(command) - 1] = '\0';

        const char* reply = amodem_send(mModem, command);
        ASSERT_NE(nullptr, reply);
        if (kData[i].FullMatch) {
            ASSERT_STREQ(kData[i].ExpectedReply, reply);
        } else {
            ASSERT_EQ(reply, strstr(reply, kData[i].ExpectedReply));
        }
        closeLogicalChannel(channel);
    }

    // Also test that transmission fails on a closed channel
    int channel = openLogicalChannel("A00000015141434C00");
    ASSERT_GE(channel, 1);

    // First transmit something successfully, then repeat the exact same thing
    // but with the channel closed
    for (int i = 0; i < 2; ++i) {
        char command[1024];
        snprintf(command, sizeof(command), "AT+CGLA=%d,10,80CAFF4000", channel);
        command[sizeof(command) - 1] = '\0';
        const char* reply = amodem_send(mModem, command);
        if (i == 0) {
            ASSERT_STREQ(kValidReply, reply);
            closeLogicalChannel(channel);
        } else {
            ASSERT_EQ(reply, strstr(reply, kCmeError));
        }
    }
}
