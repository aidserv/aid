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

#include "android/base/misc/Utf8Utils.h"

#include <gtest/gtest.h>

#define ARRAYLEN(x)  (sizeof(x)/sizeof(x[0]))

namespace android {
namespace base {

#define DATA(expected, text) { expected, text, sizeof(text) - 1U }

TEST(Utf8Utils, utf8IsValid) {
    static const struct {
        bool expected;
        const char* text;
        size_t textLen;
    } kData[] = {
        DATA(true, ""),
        DATA(true, "Hello World!"),
        DATA(true, "T\xC3\xA9l\xC3\xA9vision"),
        DATA(false, "\x80\x80"),
        DATA(true, "\xC1\xB6"),
        DATA(false, "\xC0\x7f"),
        DATA(true, "foo\xE0\x80\x80"),
        DATA(false, "foo\xE0\x80"),
    };
    const size_t kDataSize = ARRAYLEN(kData);
    for (size_t n = 0; n < kDataSize; ++n) {
        EXPECT_EQ(kData[n].expected,
                  utf8IsValid(kData[n].text,
                              kData[n].textLen)) << "#" << n;
    }
}

TEST(Utf8Utils, utf8Decode) {
    static const struct {
        int expected_len;
        uint32_t expected_codepoint;
        uint8_t text[16];
        int text_len;
    } kData[] = {
        { 0, 0, { 0, }, 0 },  // empty input returns 0
        { 1, 0, { 0, }, 1 },  // input is just 0
        { 1, 127, { 0x7f, }, 1 },
        { 2, 0, { 0xc0, 0x80 }, 2 },
        { 2, 0x80, { 0xc2, 0x80 }, 2 },
        { 2, 0x81, { 0xc2, 0x81 }, 2 },
        { 2, 0x7ff, { 0xdf, 0xbf }, 2 },
        { 3, 0x800, { 0xe0, 0xa0, 0x80 }, 3 },
        { 3, 0x7fff, { 0xe7, 0xbf, 0xbf }, 3 },
        { 3, 0xffff, { 0xef, 0xbf, 0xbf }, 3 },
        { 4, 0x10000, { 0xf0, 0x90, 0x80, 0x80 }, 4 },
        { 4, 0x1fffff, { 0xf7, 0xbf, 0xbf, 0xbf }, 4 },
        { 1, 32, { 32, 0, }, 32 },
    };
    const size_t kDataSize = ARRAYLEN(kData);
    for (size_t n = 0; n < kDataSize; ++n) {
        uint32_t codepoint = 0;
        int len = utf8Decode(kData[n].text, kData[n].text_len, &codepoint);
        EXPECT_EQ(kData[n].expected_len, len) << "#" << n;
        EXPECT_EQ(kData[n].expected_codepoint, codepoint) << "#" << n;
    }
}

TEST(Utf8Utils, utf8Encode) {
    static const struct {
        int expected_len;
        uint8_t expected_text[32];
        uint32_t codepoint;
    } kData[] = {
        { -1, { 0, }, 0x80000000U },
        { 1, { 0, 0 }, 0 },
        { 1, { 32, }, 32 },
        { 1, { 127, }, 127 },
        { 2, { 0xc2, 0x80, }, 0x80 },
        { 2, { 0xc2, 0x81, }, 0x81 },
        { 2, { 0xdf, 0xbf, }, 0x7ff },
        { 3, { 0xe0, 0xa0, 0x80 }, 0x800 },
        { 3, { 0xe7, 0xbf, 0xbf }, 0x7fff },
        { 3, { 0xef, 0xbf, 0xbf }, 0xffff },
        { 4, { 0xf0, 0x90, 0x80, 0x80 }, 0x10000 },
        { 4, { 0xf7, 0xbf, 0xbf, 0xbf }, 0x1fffff },
    };
    const size_t kDataSize = ARRAYLEN(kData);
    for (size_t n = 0; n < kDataSize; ++n) {
        uint8_t buffer[32] = { 0, };

        // First, check length without an output buffer.
        int len = utf8Encode(kData[n].codepoint, NULL, 0);
        EXPECT_EQ(kData[n].expected_len, len) << "#" << n;

        // Second, check length with an output buffer.
        len = utf8Encode(kData[n].codepoint, buffer, sizeof(buffer));
        EXPECT_EQ(kData[n].expected_len, len) << "#" << n;
        for (int ii = 0; ii < len; ++ii) {
            EXPECT_EQ(kData[n].expected_text[ii], buffer[ii]) 
                    << "#" << n << " @" << ii;
        }

        // Third, check length with a buffer that is too short.
        if (kData[n].expected_len > 0) {
            len = utf8Encode(kData[n].codepoint, buffer, (size_t)(len - 1));
            EXPECT_EQ(-1, len) << "#" << n;
        }
    }
}

}  // namespace base
}  // namespace android
