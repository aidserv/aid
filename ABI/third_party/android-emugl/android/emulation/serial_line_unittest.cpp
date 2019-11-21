// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/emulation/serial_line.h"
#include "android/emulation/SerialLine.h"

#include <gtest/gtest.h>

using android::SerialLine;

namespace {

class TestSerialLine : public SerialLine {
public:
    virtual void addHandlers(void* opaque, CanReadFunc canReadFunc, ReadFunc readFunc) {
        ++addHandlersCallCount;

        EXPECT_EQ(expectedOpaque, opaque);
        EXPECT_EQ(expectedCanReadFunc, canReadFunc);
        EXPECT_EQ(expectedReadFunc, readFunc);
    }

    virtual int write(const uint8_t* data, int len) {
        ++writeCallCount;

        EXPECT_EQ(expectedData, data);
        EXPECT_EQ(expectedLen, len);

        return writeReturn;
    }

    int addHandlersCallCount = 0;
    int writeCallCount = 0;

    void* expectedOpaque = nullptr;
    CanReadFunc expectedCanReadFunc = nullptr;
    ReadFunc expectedReadFunc = nullptr;

    const uint8_t* expectedData = nullptr;
    int expectedLen = 0;
    int writeReturn = 0;
};

}  // namespace

TEST(SerialLine, write) {
    TestSerialLine sl;
    sl.expectedLen = 1;
    uint8_t data;
    sl.expectedData = &data;
    sl.writeReturn = 2;

    const int res = android_serialline_write(&sl, &data, 1);

    EXPECT_EQ(2, res);

    EXPECT_EQ(1, sl.writeCallCount);
    EXPECT_EQ(0, sl.addHandlersCallCount);
}

// some test callbacks
static int testCanRead(void*) {
    return 0;
}
static void testRead(void*, const uint8_t*, int) {
}

TEST(SerialLine, addHandlers) {
    TestSerialLine sl;
    int dummy;
    sl.expectedOpaque = &dummy;
    sl.expectedCanReadFunc = &testCanRead;
    sl.expectedReadFunc = &testRead;

    android_serialline_addhandlers(&sl, &dummy, testCanRead, testRead);

    EXPECT_EQ(1, sl.addHandlersCallCount);
    EXPECT_EQ(0, sl.writeCallCount);
}
