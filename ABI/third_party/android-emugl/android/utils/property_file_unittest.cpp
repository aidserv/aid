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

#include "android/utils/property_file.h"

#include <stdlib.h>

#include <gtest/gtest.h>

// Unlike std::string, accept NULL as input.
class String {
public:
    explicit String(const char* str) : mStr(str) {}
    ~String() { free(const_cast<char*>(mStr)); }
    const char* str() const { return mStr; }
private:
    const char* mStr;
};

TEST(PropertyFile, EmptyFile) {
    EXPECT_FALSE(propertyFile_getValue("", 0U, "sdk.version"));
}

TEST(PropertyFile, SingleLineFile) {
    static const char kFile[] = "foo=bar\n";
    String value(propertyFile_getValue(kFile, sizeof kFile, "foo"));
    EXPECT_TRUE(value.str());
    EXPECT_STREQ("bar", value.str());

    String value2(propertyFile_getValue(kFile, sizeof kFile, "bar"));
    EXPECT_FALSE(value2.str());
}

TEST(PropertyFile, SingleLineFileWithZeroTerminator) {
    static const char kFile[] = "foo=bar";
    String value(propertyFile_getValue(kFile, sizeof kFile, "foo"));
    EXPECT_TRUE(value.str());
    EXPECT_STREQ("bar", value.str());

    String value2(propertyFile_getValue(kFile, sizeof kFile, "bar"));
    EXPECT_FALSE(value2.str());
}

TEST(PropertyFile, MultiLineFile) {
    static const char kFile[] =
        "foo=bar\n"
        "bar=zoo\n"
        "sdk=4.2\n";

    String foo(propertyFile_getValue(kFile, sizeof kFile, "foo"));
    String bar(propertyFile_getValue(kFile, sizeof kFile, "bar"));
    String sdk(propertyFile_getValue(kFile, sizeof kFile, "sdk"));

    EXPECT_STREQ("bar", foo.str());
    EXPECT_STREQ("zoo", bar.str());
    EXPECT_STREQ("4.2", sdk.str());
}

TEST(PropertyFile, MultiLineFileWithZeroTerminator) {
    static const char kFile[] =
        "foo=bar\n"
        "bar=zoo\n"
        "sdk=4.2";

    String foo(propertyFile_getValue(kFile, sizeof kFile, "foo"));
    String bar(propertyFile_getValue(kFile, sizeof kFile, "bar"));
    String sdk(propertyFile_getValue(kFile, sizeof kFile, "sdk"));

    EXPECT_STREQ("bar", foo.str());
    EXPECT_STREQ("zoo", bar.str());
    EXPECT_STREQ("4.2", sdk.str());
}

TEST(PropertyFile, MultiLineFileWithCRLF) {
    static const char kFile[] =
        "foo=bar\r\n"
        "bar=zoo\r\n"
        "sdk=4.2\n";

    String foo(propertyFile_getValue(kFile, sizeof kFile, "foo"));
    String bar(propertyFile_getValue(kFile, sizeof kFile, "bar"));
    String sdk(propertyFile_getValue(kFile, sizeof kFile, "sdk"));

    EXPECT_STREQ("bar", foo.str());
    EXPECT_STREQ("zoo", bar.str());
    EXPECT_STREQ("4.2", sdk.str());
}

TEST(PropertyFile, SpaceAroundPropertyNameAndValues) {
    static const char kFile[] = "  foo = bar zoo \n";
    String foo(propertyFile_getValue(kFile, sizeof kFile, "foo"));
    EXPECT_STREQ(" bar zoo ", foo.str());
}

TEST(PropertyFile, ValueTooLongIsTruncated) {
    static const char kFile[] =
        "foo=This is an incredible long value that is going to be truncated"
        " by the property file parser since it is longer than "
        "MAX_PROPERTY_VALUE_LEN\n";
    char expected[MAX_PROPERTY_VALUE_LEN];
    memcpy(expected, kFile + 4, sizeof expected);
    expected[(sizeof expected) - 1U] = 0;

    String foo(propertyFile_getValue(kFile, sizeof kFile, "foo"));
    EXPECT_STREQ(expected, foo.str());
}

TEST(PropertyFile, ReturnLatestVariableDefinition) {
    static const char kFile[] = "foo=bar\nfoo=zoo\n";
    String foo(propertyFile_getValue(kFile, sizeof kFile, "foo"));
    EXPECT_STREQ("zoo", foo.str());
}

TEST(PropertyFile, Iterator) {
    static const char kFile[] =
            "foo=bar\n"
            "this-name-is-too-long-and-will-be-ignored-by-the-parser=ahah\n"
            "foo2=this-value-is-too-long-and-will-be-truncated-by-the-parser"
                    "-which-only-wants-something-smaller-than-92-bytes\n"
            "foo3=bar\r\n"
            "bar= zoo";

    PropertyFileIterator iter[1];
    propertyFileIterator_init(iter, kFile, sizeof kFile);

    EXPECT_TRUE(propertyFileIterator_next(iter));
    EXPECT_STREQ("foo", iter->name);
    EXPECT_STREQ("bar", iter->value);

    EXPECT_TRUE(propertyFileIterator_next(iter));
    EXPECT_STREQ("foo2", iter->name);
    EXPECT_STREQ("this-value-is-too-long-and-will-be-truncated-by-the-"
                 "parser-which-only-wants-something-small", iter->value);

    EXPECT_TRUE(propertyFileIterator_next(iter));
    EXPECT_STREQ("foo3", iter->name);
    EXPECT_STREQ("bar", iter->value);

    EXPECT_TRUE(propertyFileIterator_next(iter));
    EXPECT_STREQ("bar", iter->name);
    EXPECT_STREQ(" zoo", iter->value);

    EXPECT_FALSE(propertyFileIterator_next(iter));
}
