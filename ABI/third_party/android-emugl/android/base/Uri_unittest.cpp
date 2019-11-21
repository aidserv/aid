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

#include "android/base/Uri.h"

#include <gtest/gtest.h>

namespace android {
namespace base {

TEST(Uri, Encode) {
    EXPECT_STREQ("", Uri::Encode("").c_str());
    EXPECT_STREQ("%21%23%24%26%27%28%29%2A%2B%2C%2F%3A%3B%3D%3F%40%5B%5D",
                 Uri::Encode("!#$&'()*+,/:;=?@[]").c_str());
    EXPECT_STREQ("%25", Uri::Encode("%").c_str());
    EXPECT_STREQ("%20", Uri::Encode(" ").c_str());
    EXPECT_STREQ("%20%20%20..%20%20", Uri::Encode("   ..  ").c_str());
    EXPECT_STREQ("pack%20my%20box%20with%20five%20dozen%20liqor%20jugs",
                 Uri::Encode("pack my box with five dozen liqor jugs").c_str());
    EXPECT_STREQ(
            "THE%20WIZARD%20QUICKLY%20JINXED%20THE%20GNOMES%20BEFORE"
            "%20THEY%20VAPORIZED.",
            Uri::Encode("THE WIZARD QUICKLY JINXED THE GNOMES BEFORE"
                        " THEY VAPORIZED.")
                    .c_str());
}

TEST(Uri, Decode) {
    EXPECT_STREQ("", Uri::Encode("").c_str());
    EXPECT_STREQ("!#$&'()*+,/:;=?@[]",
                 Uri::Decode("%21%23%24%26%27%28%29%2A%2B%2C%2F%3A%3B%3D%3F"
                             "%40%5B%5D")
                         .c_str());
    EXPECT_STREQ("%", Uri::Decode("%25").c_str());
    EXPECT_STREQ(" ", Uri::Decode("%20").c_str());
    EXPECT_STREQ("   ..  ", Uri::Decode("%20%20%20..%20%20").c_str());
    EXPECT_STREQ("pack my box with five dozen liqor jugs",
                 Uri::Decode("pack%20my%20box%20with%20five%20dozen%20liqor"
                             "%20jugs")
                         .c_str());
    EXPECT_STREQ("THE WIZARD QUICKLY JINXED THE GNOMES BEFORE THEY VAPORIZED.",
                 Uri::Decode("THE%20WIZARD%20QUICKLY%20JINXED%20THE%20GNOMES"
                             "%20BEFORE%20THEY%20VAPORIZED.")
                         .c_str());
}

TEST(Uri, FormatEncodeArguments) {
    EXPECT_STREQ("", Uri::FormatEncodeArguments("").c_str());
    EXPECT_STREQ("aa=b", Uri::FormatEncodeArguments("%s=%s", "aa", "b").c_str());
    EXPECT_STREQ("1=2&%26=%3D", Uri::FormatEncodeArguments("%d=%u&%s=%s",
                                                           1, 2u, "&", "=").c_str());
}


}  // namespace base
}  // namespace android
