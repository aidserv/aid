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

#include "android/base/StringParse.h"

#include "android/base/threads/FunctorThread.h"
#include "android/base/testing/Utils.h"
#include <gtest/gtest.h>

namespace android {
namespace base {

static void testScanf() {
    static const char comma[] = "1,3";
    static const char dot[] = "1.3";
    static const char format[] = "%f%n";
    float val = 0;
    int n = 0;

    // Now make sure we parse floating point strings as expected.
    EXPECT_EQ(1, sscanf(dot, format, &val, &n));
    EXPECT_FLOAT_EQ(1, val);
    EXPECT_EQ(1, n);
    EXPECT_EQ(1, sscanf(comma, format, &val, &n));
    EXPECT_FLOAT_EQ(1.3, val);
    EXPECT_EQ(3, n);

    // C-Locale parsing should be able to parse strings with C locale
    EXPECT_EQ(1, SscanfWithCLocale(dot, format, &val, &n));
    EXPECT_FLOAT_EQ(1.3, val);
    EXPECT_EQ(3, n);

    // And the regular parsing still works as it used to.
    EXPECT_EQ(1, sscanf(dot, format, &val, &n));
    EXPECT_FLOAT_EQ(1, val);
    EXPECT_EQ(1, n);
    EXPECT_EQ(1, sscanf(comma, format, &val, &n));
    EXPECT_FLOAT_EQ(1.3, val);
    EXPECT_EQ(3, n);
}

TEST(StringParse, SscanfWithCLocale) {
    auto scopedCommaLocale = setScopedCommaLocale();
    testScanf();
}

TEST(StringParse, SscanfWithCLocaleThreads) {
    auto scopedCommaLocale = setScopedCommaLocale();

    std::vector<std::unique_ptr<FunctorThread>> threads;
    for (int i = 0; i < 20; ++i) {
        threads.emplace_back(new FunctorThread(&testScanf));
    }
    for (auto& t : threads) {
        ASSERT_TRUE(t->start());
    }
    for (auto& t : threads) {
        ASSERT_TRUE(t->wait());
    }
}

}  // namespace base
}  // namespace android
