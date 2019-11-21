
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

#include "android/base/Version.h"

#include <gtest/gtest.h>

namespace android {
namespace base {

TEST(Version, NumberCtor) {
    Version v1(1,1,1);
    EXPECT_STREQ("1.1.1", v1.toString().c_str());

    Version v2(1,1,0);
    EXPECT_STREQ("1.1.0", v2.toString().c_str());

    Version v3(1,1,0,12);
    EXPECT_STREQ("1.1.0-12", v3.toString().c_str());
}

TEST(Version, Compare) {
    Version v(1, 1, 1);

    EXPECT_NE(v, Version(1, 1, 0));
    EXPECT_NE(v, Version(1, 0, 1));
    EXPECT_NE(v, Version(0, 1, 1));
    EXPECT_NE(v, Version(1, 1, 1, 1));

    EXPECT_EQ(v, Version(1, 1, 1));
    EXPECT_EQ(Version(1, 1, 1, 2), Version(1, 1, 1, 2));

    EXPECT_LT(v, Version(2, 1, 1));
    EXPECT_LT(v, Version(1, 2, 1));
    EXPECT_LT(v, Version(1, 1, 2));
    EXPECT_LT(v, Version::invalid());

    EXPECT_LT(v, Version(1, 1, 1, 1));
}

TEST(Version, StringCtor) {
    Version v1("1.1.1");
    EXPECT_EQ(Version(1,1,1), v1);

    Version v2("12.1.0");
    EXPECT_EQ(Version(12,1,0), v2);

    EXPECT_EQ(Version(12,1,0), Version("12.1"));
    EXPECT_EQ(Version(12,0,0), Version("12"));

    Version v3("12.0.0");
    EXPECT_EQ(Version(12,0,0), v3);

    Version v4("12.0.01");
    EXPECT_EQ(Version(12,0,1), v4);

    Version v5("12..1");
    EXPECT_EQ(Version::invalid(), v5);

    Version v6(".1");
    EXPECT_EQ(Version::invalid(), v6);

    Version v7("1.2.3.4.5");
    EXPECT_EQ(Version::invalid(), v7);

    Version v8("1.2.3.4.");
    EXPECT_EQ(Version::invalid(), v8);

    Version v9("12.foobar");
    EXPECT_EQ(Version::invalid(), v9);

    Version v10("1.2.3-4");
    EXPECT_EQ(Version(1, 2, 3, 4), v10);

    EXPECT_EQ(Version::invalid(), Version(""));
    EXPECT_EQ(Version::invalid(), Version(nullptr));
    EXPECT_EQ(Version::invalid(), Version("1.2.3.3 beta"));
    EXPECT_EQ(Version::invalid(), Version("1.2.3 preview"));
    EXPECT_EQ(Version::invalid(), Version(" 1.2.3"));
    EXPECT_EQ(Version::invalid(), Version("1.2.3 "));

    EXPECT_EQ(Version::invalid(), Version(Version::invalid().toString()));
}

TEST(Version, ToString) {
    Version v1(1,1,1);
    EXPECT_STREQ("1.1.1", v1.toString().c_str());

    Version v2(0, 0, 1);
    EXPECT_STREQ("0.0.1", v2.toString().c_str());

    Version v3(1,0,0);
    EXPECT_STREQ("1.0.0", v3.toString().c_str());

    Version v4(1,0,0,12);
    EXPECT_STREQ("1.0.0-12", v4.toString().c_str());
}

TEST(Version, Accessors) {
    Version ver(10,11,200,3030);
    EXPECT_EQ(10U, ver.component<Version::kMajor>());
    EXPECT_EQ(11U, ver.component<Version::kMinor>());
    EXPECT_EQ(200U, ver.component<Version::kMicro>());
    EXPECT_EQ(3030U, ver.component<Version::kBuild>());
}

}  // namespace base
}  // namespace android
