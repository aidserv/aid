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

#include "android/utils/format.h"
#include "android/utils/string.h"
#include "gtest/gtest.h"

#include <algorithm>
#include <stdint.h>
#include <string.h>

namespace android {
namespace utils {

static const char* dummy = "Dummy";

TEST(Utils, FormatHex) {
    char tmp[128];

    uint8_t test[] = { 0x1, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef };

    strlcpy(tmp, dummy, sizeof(tmp));
    EXPECT_EQ(0U, format_hex(tmp, sizeof(tmp), test, 0));
    EXPECT_STREQ("", tmp);

    strlcpy(tmp, dummy, sizeof(tmp));
    EXPECT_EQ(2U, format_hex(tmp, sizeof(tmp), test, 1));
    EXPECT_STREQ("01", tmp);

    strlcpy(tmp, dummy, sizeof(tmp));
    EXPECT_EQ(4U, format_hex(tmp, sizeof(tmp), test, 2));
    EXPECT_STREQ("0123", tmp);

    strlcpy(tmp, dummy, sizeof(tmp));
    EXPECT_EQ(8U, format_hex(tmp, sizeof(tmp), test, 4));
    EXPECT_STREQ("01234567", tmp);

    strlcpy(tmp, dummy, sizeof(tmp));
    EXPECT_EQ(11U, format_hex(tmp, sizeof(tmp), test, 5));
    EXPECT_STREQ("01234567 89", tmp);

    strlcpy(tmp, dummy, sizeof(tmp));
    EXPECT_EQ(17U, format_hex(tmp, sizeof(tmp), test, 8));
    EXPECT_STREQ("01234567 89abcdef", tmp);

    strlcpy(tmp, dummy, sizeof(tmp));
    EXPECT_EQ(17U, format_hex(tmp, 0, test, 8));
    EXPECT_STREQ(dummy, tmp);

    strlcpy(tmp, dummy, sizeof(tmp));
    EXPECT_EQ(17U, format_hex(tmp, 1, test, 8));
    EXPECT_STREQ("", tmp);

    strlcpy(tmp, dummy, sizeof(tmp));
    EXPECT_EQ(17U, format_hex(tmp, 2, test, 8));
    EXPECT_STREQ("0", tmp);

    strlcpy(tmp, dummy, sizeof(tmp));
    EXPECT_EQ(17U, format_hex(tmp, 3, test, 8));
    EXPECT_STREQ("01", tmp);
}

TEST(Utils, FormatPrintable) {
    char tmp[128];

    uint8_t test[] = { 0x1, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 't', 'e', 's', 't' };

    strlcpy(tmp, dummy, sizeof(tmp));
    EXPECT_EQ(0U, format_printable(tmp, sizeof(tmp), test, 0));
    EXPECT_STREQ("", tmp);

    strlcpy(tmp, dummy, sizeof(tmp));
    EXPECT_EQ(1U, format_printable(tmp, sizeof(tmp), test, 1));
    EXPECT_STREQ(".", tmp);

    strlcpy(tmp, dummy, sizeof(tmp));
    EXPECT_EQ(2U, format_printable(tmp, sizeof(tmp), test, 2));
    EXPECT_STREQ(".#", tmp);

    strlcpy(tmp, dummy, sizeof(tmp));
    EXPECT_EQ(4U, format_printable(tmp, sizeof(tmp), test, 4));
    EXPECT_STREQ(".#Eg", tmp);

    strlcpy(tmp, dummy, sizeof(tmp));
    EXPECT_EQ(5U, format_printable(tmp, sizeof(tmp), test, 5));
    EXPECT_STREQ(".#Eg.", tmp);

    strlcpy(tmp, dummy, sizeof(tmp));
    EXPECT_EQ(8U, format_printable(tmp, sizeof(tmp), test, 8));
    EXPECT_STREQ(".#Eg....", tmp);

    strlcpy(tmp, dummy, sizeof(tmp));
    EXPECT_EQ(13U, format_printable(tmp, sizeof(tmp), test, 12));
    EXPECT_STREQ(".#Eg.... test", tmp);

    strlcpy(tmp, dummy, sizeof(tmp));
    EXPECT_EQ(13U, format_printable(tmp, 0, test, 12));
    EXPECT_STREQ(dummy, tmp);

    strlcpy(tmp, dummy, sizeof(tmp));
    EXPECT_EQ(13U, format_printable(tmp, 1, test, 12));
    EXPECT_STREQ("", tmp);

    strlcpy(tmp, dummy, sizeof(tmp));
    EXPECT_EQ(13U, format_printable(tmp, 2, test, 12));
    EXPECT_STREQ(".", tmp);

    strlcpy(tmp, dummy, sizeof(tmp));
    EXPECT_EQ(13U, format_printable(tmp, 3, test, 12));
    EXPECT_STREQ(".#", tmp);
}

TEST(Utils, FormatHexPrintable) {
    char tmp[128];

    uint8_t test[] = { 0x1, 0x23, 0x45, 0x67 };

    strlcpy(tmp, dummy, sizeof(tmp));
    EXPECT_EQ(16U, format_hex_printable(tmp, sizeof(tmp), test, 4));
    EXPECT_STREQ("01234567    .#Eg", tmp);

    strlcpy(tmp, dummy, sizeof(tmp));
    EXPECT_EQ(16U, format_hex_printable(tmp, 0, test, 4));
    EXPECT_STREQ(dummy, tmp);

    strlcpy(tmp, dummy, sizeof(tmp));
    EXPECT_EQ(16U, format_hex_printable(tmp, 1, test, 4));
    EXPECT_STREQ("", tmp);

    strlcpy(tmp, dummy, sizeof(tmp));
    EXPECT_EQ(16U, format_hex_printable(tmp, 2, test, 4));
    EXPECT_STREQ("0", tmp);
}

}  // namespace utils
}  // namespace android
