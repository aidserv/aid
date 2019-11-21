// Copyright (C) 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/utils/Random.h"

#include <gtest/gtest.h>

namespace android {

TEST(Random, generateRandomBytes) {
    char buf[20];
    memset(buf, 0, sizeof(buf));

    // test what we can
    EXPECT_TRUE(generateRandomBytes(buf, 12));

    // make sure we haven't overflowed
    EXPECT_EQ(0, buf[12]);

    // confirm that all of the bytes have been effected
    for (int i = 0; i < 12; i++) {
        // if we find a 0, it might be a legit random 0, let's be sure
        while (buf[i] == 0) {
            EXPECT_TRUE(generateRandomBytes(buf, 12));
        }
    }
}

}  // namespace android
