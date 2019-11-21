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

#include "android/emulation/ParameterList.h"

#include <gtest/gtest.h>

#include <memory>
#include <string>

#include <stdlib.h>

namespace android {

TEST(ParameterList, Construction) {
    ParameterList list;
    EXPECT_EQ(0U, list.size());

    EXPECT_EQ(std::string(), list.toString());

    char* str = list.toCStringCopy();
    EXPECT_STREQ("", str);
    ::free(str);
}

TEST(ParameterList, Add) {
    ParameterList list;
    list.add("foo");
    list.add(std::string("bar"));
    list.add(std::move(std::string("zoo")));

    EXPECT_EQ(3U, list.size());
    EXPECT_STREQ("foo", list[0].c_str());
    EXPECT_STREQ("bar", list[1].c_str());
    EXPECT_STREQ("zoo", list[2].c_str());
}

TEST(ParameterList, AddFormat) {
    ParameterList list;
    list.addFormat("Hello World");
    list.addFormat("%s=%s", "foo", "bar");
    list.addFormat("%s-%s", std::string("zoo"), std::string("crab"));

    EXPECT_EQ(3U, list.size());
    EXPECT_STREQ("Hello World", list[0].c_str());
    EXPECT_STREQ("foo=bar", list[1].c_str());
    EXPECT_STREQ("zoo-crab", list[2].c_str());
}


TEST(ParameterList, Add2) {
    ParameterList list;
    list.add2("foo", "bar");
    EXPECT_EQ(2U, list.size());
    EXPECT_STREQ("foo", list[0].c_str());
    EXPECT_STREQ("bar", list[1].c_str());
}

TEST(ParameterList, AddIf) {
    ParameterList list;
    list.addIf("foo", true);
    list.addIf("bar", false);
    list.addIf("zoo", true);

    EXPECT_EQ(2U, list.size());
    EXPECT_STREQ("foo", list[0].c_str());
    EXPECT_STREQ("zoo", list[1].c_str());
}

TEST(ParameterList, Add2If) {
    ParameterList list;
    list.add2If("foo", "bar");
    list.add2If("zoo", nullptr);
    list.add2If("under", "over");

    EXPECT_EQ(4U, list.size());
    EXPECT_STREQ("foo", list[0].c_str());
    EXPECT_STREQ("bar", list[1].c_str());
    EXPECT_STREQ("under", list[2].c_str());
    EXPECT_STREQ("over", list[3].c_str());
}

TEST(ParameterList, toString) {
    ParameterList list;
    list.add2("foo", "bar");
    list.add("zoo");
    EXPECT_EQ(3U, list.size());
    EXPECT_STREQ("foo bar zoo", list.toString().c_str());
}

TEST(ParameterList, toCStringCopy) {
    ParameterList list;
    list.add2("foo", "bar");
    list.add("zoo");
    EXPECT_EQ(3U, list.size());
    char* str = list.toCStringCopy();
    EXPECT_STREQ("foo bar zoo", str);
    ::free(str);
}

TEST(ParameterList, array) {
    ParameterList list;
    list.add2("foo", "bar");
    list.add("zoo");
    EXPECT_EQ(3U, list.size());
    char** array = list.array();
    EXPECT_TRUE(array);
    EXPECT_STREQ("foo", array[0]);
    EXPECT_STREQ("bar", array[1]);
    EXPECT_STREQ("zoo", array[2]);
}

}  // namespace android
