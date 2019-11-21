/* Copyright (C) 2014-2015 The Android Open Source Project
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
#include "android/skin/keycode-buffer.h"
#include "android/skin/linux_keycodes.h"

#include <gtest/gtest.h>

#include <vector>

#define ARRAYLEN(x)  (sizeof(x)/sizeof(x[0]))

namespace android_skin {

namespace {

// Global variables used to implement the flush callback during testing.
const int kMaxCodes = 256;
int sCodes[kMaxCodes];
int sCodesCount = 0;

void myFlushFunc(int* codes, int count) {
    if (count > kMaxCodes) {
        count = kMaxCodes;
    }
    for (int n = 0; n < count; ++n) {
        sCodes[n] = codes[n];
    }
    sCodesCount = count;
}

}  // namespace

TEST(keycode_buffer,skin_keycode_buffer_flush) {
    static const struct {
        int expected;
        unsigned code;
        bool down;
    } kData[] = {
            {LINUX_KEY_F1 | 0x400, LINUX_KEY_F1, true},
            {LINUX_KEY_F1, LINUX_KEY_F1, false},
            {LINUX_KEY_COMPOSE, LINUX_KEY_COMPOSE, false},
            {LINUX_KEY_COMPOSE | 0x400, LINUX_KEY_COMPOSE, true},
    };
    const size_t kDataLen = ARRAYLEN(kData);

    SkinKeycodeBuffer buffer;

    skin_keycode_buffer_init(&buffer, &myFlushFunc);
    for (size_t nn = 0U; nn < kDataLen; ++nn) {
        skin_keycode_buffer_add(&buffer, kData[nn].code, kData[nn].down);
    }
    skin_keycode_buffer_flush(&buffer);

    EXPECT_EQ(kDataLen, static_cast<size_t>(sCodesCount));
    for (size_t nn = 0U; nn < kDataLen; ++nn) {
        EXPECT_EQ(kData[nn].expected, sCodes[nn]) << "#" << nn;
    }
}

}  // namespace android_skin
