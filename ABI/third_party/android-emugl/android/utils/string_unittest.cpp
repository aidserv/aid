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

#include "android/utils/string.h"

#include <gtest/gtest.h>

#include <stdlib.h>

#define TEST_SKIP(string, skipped) \
    EXPECT_EQ(string + skipped, str_skip_white_space_if_any(string))
TEST(String, str_skip_white_space_if_any) {
    TEST_SKIP("no change", 0);
    TEST_SKIP("", 0);
    TEST_SKIP(" left", 1);
    TEST_SKIP("right ", 0);
    TEST_SKIP("\nnewline\r", 1);
    TEST_SKIP(" \r\n\tmulti \t\r\n", 4);
}

TEST(String, str_begins_with) {
    EXPECT_TRUE(str_begins_with("12345678", "1234"));
    EXPECT_TRUE(str_begins_with("auth", "auth"));
    EXPECT_TRUE(str_begins_with("", ""));
    EXPECT_TRUE(str_begins_with("12345678", ""));
    EXPECT_FALSE(str_begins_with("12345678", "abcd"));
    EXPECT_FALSE(str_begins_with("12345678", "1234abcd"));
    EXPECT_FALSE(str_begins_with("1234", "12345678"));
    EXPECT_FALSE(str_begins_with("", "abcd"));
}

TEST(String, str_ends_withFailure) {
    EXPECT_FALSE(str_ends_with("", "suffix"));
    EXPECT_FALSE(str_ends_with("fix", "suffix"));
    EXPECT_FALSE(str_ends_with("abc", "xxyyzz"));
}

TEST(String, str_ends_withSuccess) {
    EXPECT_TRUE(str_ends_with("string", ""));
    EXPECT_TRUE(str_ends_with("", ""));
    EXPECT_TRUE(str_ends_with("abc", "abc"));
    EXPECT_TRUE(str_ends_with("abc", "c"));
    EXPECT_TRUE(str_ends_with("0123", "23"));
}

TEST(String, str_reset) {
    char* str = strdup("Foo Bar");
    const char kText[] = "Hello World";
    str_reset(&str, kText);
    EXPECT_NE(kText, str);
    EXPECT_STREQ(kText, str);
    ::free(str);
}

TEST(String, str_reset_withNULL) {
    char* str = strdup("Foo Bar");
    str_reset(&str, NULL);
    EXPECT_FALSE(str);
}

TEST(String, str_reset_nocopy) {
    char* str = strdup("Foo Bar");
    char* text = strdup("Hello World");
    str_reset_nocopy(&str, text);
    EXPECT_EQ(text, str);
    ::free(text);
}

TEST(String, str_reset_null) {
    char* str = strdup("Hello World");
    str_reset_null(&str);
    EXPECT_FALSE(str);
}
