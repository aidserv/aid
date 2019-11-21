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

#include "android/base/ArraySize.h"
#include "android/base/StringView.h"

#include <gtest/gtest.h>

namespace android {
namespace base {

TEST(StringView, InitEmpty) {
    StringView view;
    EXPECT_TRUE(view.empty());
}

TEST(StringView, InitWithCString) {
    static const char* kString = "Hello";
    StringView view(kString);
    EXPECT_STREQ(kString, view.str());
    EXPECT_FALSE(view.empty());
    EXPECT_EQ(strlen(kString), view.size());
}

TEST(StringView, InitWithConstBuffer) {
    static const char kString[] = "Hello";
    StringView view(kString);
    EXPECT_STREQ(kString, view.str());
    EXPECT_FALSE(view.empty());
    EXPECT_EQ(strlen(kString), view.size());
}

TEST(StringView, InitWithMutableBuffer) {
    char string[128] = "Hello";
    ASSERT_NE(stringLiteralLength(string), strlen(string));
    StringView view(string);
    EXPECT_STREQ(string, view.str());
    EXPECT_FALSE(view.empty());
    EXPECT_EQ(strlen(string), view.size());
}

namespace {
    // Just a test class which needs compile-time constant as an agrument
    template <int N>
    struct checkConst {
        checkConst() {
            std::cout << "N = " << N << '/';
        }
    };
}

TEST(StringView, InitConstexpr) {
    static constexpr StringView kStringView = "Hello";

    // these lines compile only if the argument is a compile-time constant
    checkConst<kStringView[0]> checker1;
    checkConst<kStringView.c_str()[1]> checker2;
    checkConst<kStringView.begin()[2]> checker3;
    checkConst<kStringView.size()> checker4;
    checkConst<kStringView.empty()> checker5;
}

TEST(StringView, InitWithStringView) {
    static const char kString[] = "Hello2";
    StringView view1(kString);
    StringView view2(view1);
    EXPECT_FALSE(view2.empty());
    EXPECT_STREQ(kString, view2.str());
    EXPECT_EQ(strlen(kString), view2.size());
}

TEST(StringView, Clear) {
    StringView view("Hello3");
    EXPECT_FALSE(view.empty());
    view.clear();
    EXPECT_TRUE(view.empty());
    EXPECT_EQ(0U, view.size());
    EXPECT_STREQ("", view.str());
}

TEST(StringView, SetEmpty) {
    StringView view("Hello4");
    view.set("");
    EXPECT_TRUE(view.empty());
}

TEST(StringView, SetEmptyWithLength) {
    StringView view("Hello5");
    view.set("Oops", 0U);
    EXPECT_TRUE(view.empty());
}

TEST(StringView, SetWithCString) {
    static const char kString[] = "Wow";
    StringView view("Hello6");
    view.set(kString);
    EXPECT_EQ(kString, view.str());
    EXPECT_EQ(strlen(kString), view.size());
}

TEST(StringView, SetWithStringView) {
    static const char kString[] = "Wazza";
    StringView view1(kString);
    StringView view("Nope");
    view.set(view1);
    EXPECT_EQ(kString, view.str());
    EXPECT_EQ(strlen(kString), view.size());
}

TEST(StringView, OperatorAt) {
    static const char kString[] = "Whatever";
    static const size_t kStringLen = sizeof(kString) - 1;
    StringView view(kString);
    for (size_t n = 0; n < kStringLen; ++n) {
        EXPECT_EQ(kString[n], view[n]) << "at index " << n;
    }
}

TEST(StringView, IsNullTerminated) {
    EXPECT_TRUE(StringView().isNullTerminated());
    EXPECT_TRUE(StringView("blah").isNullTerminated());
    EXPECT_FALSE(StringView("blah", 1).isNullTerminated());
    EXPECT_TRUE(StringView("blah", 4).isNullTerminated());
    auto sv = StringView{};
    sv.set("blah");
    EXPECT_TRUE(sv.isNullTerminated());
    sv.set("blah", 2);
    EXPECT_FALSE(sv.isNullTerminated());
    sv.set("blah", 4);
    EXPECT_TRUE(sv.isNullTerminated());
    sv.set(nullptr);
    EXPECT_TRUE(sv.isNullTerminated());
}

TEST(StringView, Iterators) {
    static const char kString[] = "What else?";
    static const size_t kStringLen = sizeof(kString) - 1;
    StringView view(kString);
    EXPECT_EQ(kString, view.begin());
    EXPECT_EQ(kString + kStringLen, view.end());

    size_t n = 0;
    for (StringView::const_iterator it = view.begin();
         it != view.end(); ++it, ++n) {
        EXPECT_EQ(kString[n], *it);
    }
}

TEST(StringView, ComparisonOperators) {
    char kHello1[] = "Hello";
    char kHello2[] = "Hello";
    StringView view1(kHello1);
    StringView view2(kHello2);
    EXPECT_TRUE(view1 == view2);
    EXPECT_FALSE(view1 != view2);
    EXPECT_TRUE(view1 <= view2);
    EXPECT_TRUE(view1 >= view2);
    EXPECT_FALSE(view1 < view2);
    EXPECT_FALSE(view1 > view2);

    StringView view3("hell");  // Shorter, but first char is larger.
    EXPECT_FALSE(view1 == view3);
    EXPECT_TRUE(view1 != view3);
    EXPECT_TRUE(view1 < view3);
    EXPECT_TRUE(view1 <= view3);
    EXPECT_FALSE(view1 > view3);
    EXPECT_FALSE(view1 >= view3);

    StringView view4("Hell");  // Shorter, but first char is smaller.
    EXPECT_FALSE(view1 == view4);
    EXPECT_TRUE(view1 != view4);
    EXPECT_FALSE(view1 < view4);
    EXPECT_FALSE(view1 <= view4);
    EXPECT_TRUE(view1 > view4);
    EXPECT_TRUE(view1 >= view4);

    EXPECT_EQ(StringView(""), StringView(""));
    EXPECT_GE(StringView(""), StringView(""));
    EXPECT_GT(StringView("1"), StringView(""));
    EXPECT_GE(StringView("1"), StringView(""));
    EXPECT_LT(StringView(""), StringView("2"));
    EXPECT_LE(StringView(""), StringView("2"));
}

TEST(StringView, Compare) {
    // empty string
    EXPECT_EQ(StringView("").compare(StringView("")), 0);
    EXPECT_GT(StringView("1").compare(StringView("")), 0);
    EXPECT_LT(StringView("").compare(StringView("2")), 0);

    // non-empty strings, same length
    EXPECT_EQ(StringView("1").compare(StringView("1")), 0);
    EXPECT_GT(StringView("2").compare(StringView("1")), 0);
    EXPECT_LT(StringView("1").compare(StringView("2")), 0);

    // non-empty strings, first shorter
    EXPECT_GT(StringView("2").compare(StringView("12")), 0);
    EXPECT_LT(StringView("1").compare(StringView("11")), 0);

    // non-empty strings, first longer
    EXPECT_GT(StringView("11").compare(StringView("1")), 0);
    EXPECT_LT(StringView("12").compare(StringView("2")), 0);
}

// TODO(digit): String

}  // namespace base
}  // namespace android
