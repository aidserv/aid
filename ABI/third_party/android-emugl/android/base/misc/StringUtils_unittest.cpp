// Copyright (C) 2015 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "android/base/misc/StringUtils.h"

#include <gtest/gtest.h>

#include <list>
#include <string>
#include <vector>

namespace android {
namespace base {

#ifdef _WIN32
TEST(StringUtils, memmem) {
    auto src = "string";

    // basic cases - when the substring is there
    EXPECT_EQ(src + 2, memmem(src, strlen(src), "ring", 4));
    EXPECT_EQ(src + 1, memmem(src, strlen(src), "tri", 3));

    // no substring
    EXPECT_EQ(nullptr, memmem(src, strlen(src), "123", 3));
    EXPECT_EQ(nullptr, memmem(src, strlen(src), "strings", 7));

    // make sure it's looking at lengths, and not at the null-terminator
    EXPECT_EQ(src + 2, memmem(src, strlen(src), "ringer", 4));
    EXPECT_EQ(nullptr, memmem(src, 2, "tr", 2));

    // corner cases - empty/single char and the needle same as haystack
    EXPECT_EQ(src, memmem(src, strlen(src), "", 0));
    EXPECT_EQ(src, memmem(src, strlen(src), "s", 1));
    EXPECT_EQ(src, memmem(src, 1, "s", 1));
    EXPECT_EQ(nullptr, memmem("", 0, "s", 1));
    EXPECT_EQ(nullptr, memmem("", 0, "", 0));
    EXPECT_EQ(src, memmem(src, strlen(src), src, strlen(src)));

    // null input parameters
    EXPECT_EQ(nullptr, memmem(src, strlen(src), nullptr, 2));
    EXPECT_EQ(nullptr, memmem(nullptr, 10, "asdf", 4));
    EXPECT_EQ(nullptr, memmem(nullptr, 10, nullptr, 1));
}
#endif // _WIN32

TEST(StringUtils, strDupWithStringView) {
    StringView view("Hello World");
    char* s = strDup(view);
    EXPECT_TRUE(s);
    EXPECT_STREQ(view.c_str(), s);
    EXPECT_NE(view.c_str(), s);
    free(s);
}

TEST(StringUtils, strDupWithStdString) {
    std::string str("Hello World");
    char* s = strDup(str);
    EXPECT_TRUE(s);
    EXPECT_STREQ(str.c_str(), s);
    EXPECT_NE(str.c_str(), s);
    free(s);
}

TEST(StringUtils, strContainsWithStringView) {
    StringView haystack("This is a long string to search for stuff");
    EXPECT_FALSE(strContains(haystack, "needle"));
    EXPECT_FALSE(strContains(haystack, "stuffy"));
    EXPECT_TRUE(strContains(haystack, "stuf"));
    EXPECT_TRUE(strContains(haystack, "stuff"));
    EXPECT_FALSE(strContains(haystack, "This is a short phrase"));
    EXPECT_TRUE(strContains(haystack, "a long string"));
}

TEST(StringUtils, strContainsWithStdString) {
    std::string haystack("This is a long string to search for stuff");
    EXPECT_FALSE(strContains(haystack, "needle"));
    EXPECT_FALSE(strContains(haystack, "stuffy"));
    EXPECT_TRUE(strContains(haystack, "stuff"));
    EXPECT_FALSE(strContains(haystack, "This is a short phrase"));
    EXPECT_TRUE(strContains(haystack, "a long string"));
}

TEST(StringUtils, join) {
    using android::base::join;

    EXPECT_STREQ("", join(std::vector<int>{}).c_str());
    EXPECT_STREQ("aa", join(std::vector<std::string>{"aa"}).c_str());
    EXPECT_STREQ("aa,bb", join(std::list<const char*>{"aa", "bb"}).c_str());
    EXPECT_STREQ("1,2,3", join(std::vector<int>{1, 2, 3}).c_str());
    EXPECT_STREQ("1|2|3", join(std::vector<int>{1, 2, 3}, '|').c_str());
    EXPECT_STREQ("1<|>2<|>sd",
                 join(std::vector<const char*>{"1", "2", "sd"}, "<|>").c_str());
    EXPECT_STREQ("...---...",
                 join(std::vector<const char*>{"...", "..."}, "---").c_str());
    EXPECT_STREQ("...---...",
                 join(std::vector<const char*>{"...", "---", "..."}, "").c_str());

    // check some special cases - lvalue modifiable and const references
    using StringVec = std::vector<std::string>;
    StringVec src = { "1", "a", "foo" };

    EXPECT_STREQ("1,a,foo", join(src).c_str());
    EXPECT_STREQ("1, a, foo", join(src, ", ").c_str());
    EXPECT_STREQ("1,a,foo", join(const_cast<const StringVec&>(src)).c_str());
    EXPECT_STREQ("1, a, foo",
                 join(const_cast<const StringVec&>(src), ", ").c_str());
}

TEST(StringUtils, trim) {
    struct {
        const char* before;
        const char* after;
    } test_data[] = {
            {"no change", "no change"},
            {"", ""},
            {" left", "left"},
            {"right ", "right"},
            {"\nnewline\r", "newline"},
            {" \r\n\tmulti \t\r\n", "multi"},
    };
    size_t TEST_DATA_COUNT = sizeof(test_data) / sizeof(test_data[0]);

    for (size_t i = 0; i < TEST_DATA_COUNT; i++) {
        std::string before(test_data[i].before);
        std::string after = trim(before);
        EXPECT_STREQ(test_data[i].after, after.c_str());
    }
}

}  // namespace base
}  // namespace android
