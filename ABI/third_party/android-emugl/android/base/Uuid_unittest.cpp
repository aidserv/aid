// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/Uuid.h"

#include <gtest/gtest.h>

namespace android {
namespace base {

TEST(Uuid, DefaultCtor) {
    Uuid uuid;
    EXPECT_EQ(Uuid(Uuid::nullUuidStr), uuid);
    EXPECT_STREQ(Uuid::nullUuidStr, uuid.toString().c_str());
}

TEST(Uuid, StringCtor) {
    static const char str[] = "01010101-0101-0101-0101-10ffac900017";
    Uuid uuid(str);
    EXPECT_STREQ(str, uuid.toString().c_str());

    uuid = Uuid("blah");
    EXPECT_EQ(Uuid(), uuid);
}

TEST(Uuid, Generate) {
    EXPECT_NE(Uuid(), Uuid::generate());
    EXPECT_NE(Uuid::generate(), Uuid::generate());
    // well, what else could we test here?
}

TEST(Uuid, Comparisons) {
    Uuid a(Uuid::nullUuidStr);
    Uuid b(Uuid::nullUuidStr);

    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
    EXPECT_FALSE(a < b);
    EXPECT_FALSE(a > b);

    b = Uuid("01010101-0101-0101-0101-10ffac900017");
    EXPECT_FALSE(a == b);
    EXPECT_TRUE(a != b);
    EXPECT_TRUE(a < b);
    EXPECT_FALSE(a > b);
}

}  // namespace base
}  // namespace android
