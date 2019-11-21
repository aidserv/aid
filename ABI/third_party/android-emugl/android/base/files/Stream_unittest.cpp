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

#include "android/base/files/Stream.h"

#include <gtest/gtest.h>

#include <string.h>

#define ARRAYLEN(x)  (sizeof(x)/sizeof(x[0]))

namespace android {
namespace base {

// A small Stream implementation used during this test. It uses a
// client-provided fixed-size buffer to store the data, which can be
// used either for reading or writing.
class MemoryStream : public Stream {
public:
    MemoryStream(void* buffer, size_t bufferLen) :
            mBuffer(static_cast<uint8_t*>(buffer)),
            mSize(bufferLen),
            mPos(0) {}

    MemoryStream(const void* buffer, size_t bufferLen) :
            mBuffer(static_cast<uint8_t*>(const_cast<void*>(buffer))),
            mSize(bufferLen),
            mPos(0) {}

    virtual ssize_t read(void* buffer, size_t len) {
        size_t avail = mSize - mPos;
        if (len > avail) {
            len = avail;
        }
        ::memcpy(buffer, mBuffer + mPos, len);
        mPos += len;
        return static_cast<ssize_t>(len);
    }

    virtual ssize_t write(const void* buffer, size_t len) {
        size_t avail = mSize - mPos;
        if (len > avail) {
            len = avail;
        }
        ::memcpy(mBuffer + mPos, buffer, len);
        mPos += len;
        return static_cast<ssize_t>(len);
    }

private:
    uint8_t* mBuffer;
    size_t   mSize;
    size_t   mPos;
};

TEST(Stream,getByte) {
    static const char kData[] = "Hello world!";
    MemoryStream stream(kData, sizeof(kData));
    for (size_t n = 0; n < sizeof(kData); ++n) {
        EXPECT_EQ(static_cast<uint8_t>(kData[n]), stream.getByte())
                << "#" << n;
    }
}

TEST(Stream,putByte) {
    static const char kData[] = "Hello world!";
    char buffer[sizeof(kData)];
    ::memset(buffer, 0, sizeof(buffer));
    MemoryStream stream(buffer, sizeof(buffer));
    for (size_t n = 0; n < sizeof(kData); ++n) {
        stream.putByte(static_cast<uint8_t>(kData[n]));
        EXPECT_EQ(kData[n], buffer[n]) << "#" << n;
    }
}


TEST(Stream,putBe16) {
    static const uint16_t kData[] = {
        0x1234, 0x5678, 0x9abc, 0xdef0
    };
    static const uint8_t kExpected[sizeof(kData)] = {
        0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0,
    };
    uint8_t buffer[sizeof(kExpected)] = { 0, };
    MemoryStream stream(buffer, sizeof(buffer));

    for (size_t n = 0; n < sizeof(kData) / sizeof(*kData); ++n) {
        stream.putBe16(kData[n]);
    }
    for (size_t n = 0; n < sizeof(kExpected); ++n) {
        EXPECT_EQ(kExpected[n], buffer[n]) << "#" << n;
    }
}

TEST(Stream,getBe16) {
    static const uint8_t kData[] = {
        0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0,
    };
    static const uint16_t kExpected[] = {
        0x1234, 0x5678, 0x9abc, 0xdef0
    };
    MemoryStream stream(kData, sizeof(kData));

    for (size_t n = 0; n < ARRAYLEN(kExpected); ++n) {
        EXPECT_EQ(kExpected[n], stream.getBe16()) << "#" << n;
    }
}

TEST(Stream,putBe32) {
    static const uint32_t kData[] = {
        0x12345678, 0x9abcdef0
    };
    static const uint8_t kExpected[sizeof(kData)] = {
        0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0,
    };
    uint8_t buffer[sizeof(kExpected)] = { 0, };
    MemoryStream stream(buffer, sizeof(buffer));

    for (size_t n = 0; n < sizeof(kData) / sizeof(*kData); ++n) {
        stream.putBe32(kData[n]);
    }
    for (size_t n = 0; n < sizeof(kExpected); ++n) {
        EXPECT_EQ(kExpected[n], buffer[n]) << "#" << n;
    }
}

TEST(Stream,getBe32) {
    static const uint8_t kData[] = {
        0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0,
    };
    static const uint32_t kExpected[] = {
        0x12345678, 0x9abcdef0
    };
    MemoryStream stream(kData, sizeof(kData));

    for (size_t n = 0; n < ARRAYLEN(kExpected); ++n) {
        EXPECT_EQ(kExpected[n], stream.getBe32()) << "#" << n;
    }
}

TEST(Stream,putBe64) {
    static const uint64_t kData[] = {
        0x123456789abcdef0
    };
    static const uint8_t kExpected[sizeof(kData)] = {
        0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0,
    };
    uint8_t buffer[sizeof(kExpected)] = { 0, };
    MemoryStream stream(buffer, sizeof(buffer));

    for (size_t n = 0; n < sizeof(kData) / sizeof(*kData); ++n) {
        stream.putBe64(kData[n]);
    }
    for (size_t n = 0; n < sizeof(kExpected); ++n) {
        EXPECT_EQ(kExpected[n], buffer[n]) << "#" << n;
    }
}

TEST(Stream,getBe64) {
    static const uint8_t kData[] = {
        0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0,
    };
    static const uint64_t kExpected[] = {
        0x123456789abcdef0
    };
    MemoryStream stream(kData, sizeof(kData));

    for (size_t n = 0; n < ARRAYLEN(kExpected); ++n) {
        EXPECT_EQ(kExpected[n], stream.getBe64()) << "#" << n;
    }
}

TEST(Stream, putFloat) {
    static const float kValue = 3.141592;
    static const uint8_t kExpected[] = {
        0xd8, 0x0f, 0x49, 0x40,
    };
    uint8_t buffer[sizeof(kExpected)] = { 0, };
    MemoryStream stream(buffer, sizeof(buffer));

    stream.putFloat(kValue);
    for (size_t n = 0; n < sizeof(kExpected); ++n) {
        EXPECT_EQ(kExpected[n], buffer[n]) << "#" << n;
    }
}

TEST(Stream, getFloat) {
    static const uint8_t kData[] = {
        0xd8, 0x0f, 0x49, 0x40,
    };
    static const float kExpected = 3.141592;
    MemoryStream stream(kData, sizeof(kData));
    EXPECT_EQ(kExpected, stream.getFloat());
}

TEST(Stream, putStringWithBaseString) {
    static const char kInput[] = "Hello world";
    static const uint8_t kExpected[] = {
        0x00, 0x00, 0x00, 0x0b,
        'H', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd',
    };
    uint8_t buffer[32];
    MemoryStream stream(buffer, sizeof(buffer));
    stream.putString(std::string(kInput));
    for (size_t n = 0; n < sizeof(kExpected); ++n) {
        EXPECT_EQ(kExpected[n], buffer[n]) << "#" << n;
    }
}

TEST(Stream, putStringWithCString) {
    static const char kInput[] = "Hello world";
    static const uint8_t kExpected[] = {
        0x00, 0x00, 0x00, 0x0b,
        'H', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd',
    };
    uint8_t buffer[32];
    MemoryStream stream(buffer, sizeof(buffer));
    stream.putString(kInput);
    for (size_t n = 0; n < sizeof(kExpected); ++n) {
        EXPECT_EQ(kExpected[n], buffer[n]) << "#" << n;
    }
}

TEST(Stream, putString) {
    static const char kInput[] = "Hello world";
    static const uint8_t kExpected[] = {
        0x00, 0x00, 0x00, 0x0b,
        'H', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd',
    };
    uint8_t buffer[32];
    MemoryStream stream(buffer, sizeof(buffer));
    stream.putString(kInput, sizeof(kInput) - 1U);
    for (size_t n = 0; n < sizeof(kExpected); ++n) {
        EXPECT_EQ(kExpected[n], buffer[n]) << "#" << n;
    }
}

TEST(Stream, getString) {
    static const uint8_t kInput[] = {
        0x00, 0x00, 0x00, 0x0b,
        'H', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd',
    };
    static const char kExpected[] = "Hello world";
    MemoryStream stream(kInput, sizeof(kInput));
    std::string str = stream.getString();
    EXPECT_EQ(sizeof(kExpected) - 1U, str.size());
    EXPECT_STREQ(kExpected, str.c_str());
}

}  // namespace base
}  // namespace android
