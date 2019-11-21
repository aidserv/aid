/* Copyright (C) 2014 The Android Open Source Project
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

#include "android/skin/keycode.h"

#include <gtest/gtest.h>

namespace android_skin {

#define ARRAYLEN(x)  (sizeof(x)/sizeof(x[0]))

namespace {

// Shared table of key pair samples for testing string mapping functions.
const struct {
    const char* string;
    uint32_t code;
    uint32_t mod;
} kKeyPairData[] = {
        {"F1", LINUX_KEY_F1, 0},
        {"Ctrl-F2", LINUX_KEY_F2, kKeyModLCtrl},
        {"RCtrl-A", LINUX_KEY_A, kKeyModRCtrl},
        {"Alt-Return", LINUX_KEY_ENTER, kKeyModLAlt},
        {"RAlt-Comma", LINUX_KEY_COMMA, kKeyModRAlt},
        {"Shift-3", LINUX_KEY_3, kKeyModLShift},
        {"RShift-Keypad_5", LINUX_KEY_KP5, kKeyModRShift},
        {"Ctrl-Shift-Alt-Delete", LINUX_KEY_DELETE,
         kKeyModLCtrl | kKeyModLShift | kKeyModLAlt},
};

const size_t kKeyPairDataLen = ARRAYLEN(kKeyPairData);

}  // namespace


TEST(keycode,skin_keycode_rotate) {
    static const struct {
        SkinKeyCode expected;
        SkinKeyCode code;
        int rotation;
    } kData[] = {
        { kKeyCodeDpadUp, kKeyCodeDpadUp, 0 },
        { kKeyCodeDpadRight, kKeyCodeDpadUp, 1 },
        { kKeyCodeDpadDown, kKeyCodeDpadUp, 2 },
        { kKeyCodeDpadLeft, kKeyCodeDpadUp, 3 },

        { kKeyCodeDpadLeft, kKeyCodeDpadLeft, 0 },
        { kKeyCodeDpadUp, kKeyCodeDpadLeft, 1 },
        { kKeyCodeDpadRight, kKeyCodeDpadLeft, 2 },
        { kKeyCodeDpadDown, kKeyCodeDpadLeft, 3 },

        { kKeyCodeDpadDown, kKeyCodeDpadDown, 0 },
        { kKeyCodeDpadLeft, kKeyCodeDpadDown, 1 },
        { kKeyCodeDpadUp, kKeyCodeDpadDown, 2 },
        { kKeyCodeDpadRight, kKeyCodeDpadDown, 3 },

        { kKeyCodeDpadRight, kKeyCodeDpadRight, 0 },
        { kKeyCodeDpadDown, kKeyCodeDpadRight, 1 },
        { kKeyCodeDpadLeft, kKeyCodeDpadRight, 2 },
        { kKeyCodeDpadUp, kKeyCodeDpadRight, 3 },

        { kKeyCodeA, kKeyCodeA, 0 },
        { kKeyCodeA, kKeyCodeA, 1 },
        { kKeyCodeA, kKeyCodeA, 2 },
        { kKeyCodeA, kKeyCodeA, 3 },
    };
    const size_t kDataLen = ARRAYLEN(kData);

    for (size_t nn = 0; nn < kDataLen; ++nn) {
        SkinKeyCode code = kData[nn].code;
        int rotation = kData[nn].rotation;
        EXPECT_EQ(kData[nn].expected, skin_keycode_rotate(code, rotation))
                << "code=" << code << " rotation=" << rotation;
    }
}

TEST(keycode, skin_key_pair_to_string) {
    for (size_t nn = 0; nn < kKeyPairDataLen; ++nn) {
        uint32_t code = kKeyPairData[nn].code;
        uint32_t mod = kKeyPairData[nn].mod;
        EXPECT_STREQ(kKeyPairData[nn].string,
                     skin_key_pair_to_string(code, mod))
                << "code=" << code << " mod=" << mod;
    }
}

TEST(keycode, skin_key_pair_from_string) {
    for (size_t nn = 0; nn < kKeyPairDataLen; ++nn) {
        uint32_t code = 0;
        uint32_t mod = 0;
        EXPECT_TRUE(skin_key_pair_from_string(
                kKeyPairData[nn].string,
                &code,
                &mod)) << "#" << nn;
        EXPECT_EQ(kKeyPairData[nn].code, code);
        EXPECT_EQ(kKeyPairData[nn].mod, mod);
    }
}

}  // namespace android_skin
