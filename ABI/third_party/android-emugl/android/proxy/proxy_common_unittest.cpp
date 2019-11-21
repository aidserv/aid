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

extern "C" {
#include "android/proxy/proxy_int.h"
}

#include <gtest/gtest.h>

TEST(ProxyCommon, proxy_base64_encode) {
    char dst[64];

    EXPECT_EQ(0, proxy_base64_encode("", 0, dst, 64));

    EXPECT_EQ(4, proxy_base64_encode("\0", 1, dst, 64));
    dst[4] = 0;
    EXPECT_STREQ("AA==", dst);

    EXPECT_EQ(4, proxy_base64_encode("\xff", 1, dst, 64));
    dst[4] = 0;
    EXPECT_STREQ("/w==", dst);

    EXPECT_EQ(8, proxy_base64_encode("\xaa\xbb\xcc\xdd", 4, dst, 64));
    dst[8] = 0;
    EXPECT_STREQ("qrvM3Q==", dst);
}
