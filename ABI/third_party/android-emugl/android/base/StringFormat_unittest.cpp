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

#include "android/base/StringFormat.h"

#include <gtest/gtest.h>

namespace android {
namespace base {

TEST(StringFormat, EmptyString) {
    std::string s = StringFormat("");
    EXPECT_TRUE(s.empty());
    EXPECT_STREQ("", s.c_str());
}

TEST(StringFormat, SimpleString) {
    std::string s = StringFormat("foobar");
    EXPECT_STREQ("foobar", s.c_str());
}

TEST(StringFormat, SimpleDecimal) {
    std::string s = StringFormat("Pi is about %d.%d\n", 3, 1415);
    EXPECT_STREQ("Pi is about 3.1415\n", s.c_str());
}

TEST(StringFormat, VeryLongString) {
    static const char kPiece[] = "A hospital bed is a parked taxi with the meter running - Groucho Marx. ";
    const size_t kPieceLen = sizeof(kPiece) - 1U;
    std::string s = StringFormat("%s%s%s%s%s%s%s", kPiece, kPiece, kPiece,
                                 kPiece, kPiece, kPiece, kPiece);
    EXPECT_EQ(7U * kPieceLen, s.size());
    for (size_t n = 0; n < 7U; ++n) {
        std::string s2 = std::string(s.c_str() + n * kPieceLen, kPieceLen);
        EXPECT_STREQ(kPiece, s2.c_str()) << "Index #" << n;
    }
}

TEST(StringAppendFormat, EmptyString) {
    std::string s = "foo";
    StringAppendFormat(&s, "");
    EXPECT_EQ(3U, s.size());
    EXPECT_STREQ("foo", s.c_str());
}

TEST(StringAppendFormat, SimpleString) {
    std::string s = "foo";
    StringAppendFormat(&s, "bar");
    EXPECT_EQ(6U, s.size());
    EXPECT_STREQ("foobar", s.c_str());
}

TEST(StringAppendFormat, VeryLongString) {
    static const char kPiece[] = "A hospital bed is a parked taxi with the meter running - Groucho Marx. ";
    const size_t kPieceLen = sizeof(kPiece) - 1U;
    const size_t kCount = 12;
    std::string s;
    for (size_t n = 0; n < kCount; ++n) {
        StringAppendFormat(&s, "%s", kPiece);
    }

    EXPECT_EQ(kCount * kPieceLen, s.size());
    for (size_t n = 0; n < kCount; ++n) {
        std::string s2 = std::string(s.c_str() + n * kPieceLen, kPieceLen);
        EXPECT_STREQ(kPiece, s2.c_str()) << "Index #" << n;
    }
}

}  // namespace base
}  // namespace android
