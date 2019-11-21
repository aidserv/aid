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

#include "android/utils/aconfig-file.h"

#include <gtest/gtest.h>

#include <stddef.h>

namespace {

class ScopedAConfig {
public:
    ScopedAConfig() : mNode(NULL) {}

    explicit ScopedAConfig(AConfig* node) : mNode(node) {}

    ~ScopedAConfig() {
        if (mNode) {
            aconfig_node_free(mNode);
        }
    }

    bool isValid() const { return mNode != NULL; }

    AConfig* get() const { return mNode; }

private:
    AConfig* mNode;
};

}  // namespace

TEST(AConfigFile, Empty) {
    ScopedAConfig node(aconfig_node("", ""));
    EXPECT_TRUE(node.isValid());
    EXPECT_FALSE(aconfig_find(node.get(), "flag"));
}

TEST(AConfigFile, Set) {
    ScopedAConfig node(aconfig_node("", ""));

    aconfig_set(node.get(), "foo", "123");
    EXPECT_EQ(123, aconfig_int(node.get(), "foo", 678));

    aconfig_set(node.get(), "foo", "-789");
    EXPECT_EQ(-789, aconfig_int(node.get(), "foo", 999));

    aconfig_set(node.get(), "bar.foo", "111");
    EXPECT_EQ(-789, aconfig_int(node.get(), "foo", 999));
    EXPECT_EQ(111, aconfig_int(node.get(), "bar.foo", 999));
}

TEST(AConfigFile, SimpleValues) {
    ScopedAConfig node(aconfig_node("", ""));

    char kData[] =
        "integer -1234\n"
        "boolean true\n"
        "unsigned 128\n"
        "string  This is a string";

    aconfig_load(node.get(), kData);
    EXPECT_EQ(-1234, aconfig_int(node.get(), "integer", 0));
    EXPECT_TRUE(aconfig_bool(node.get(), "boolean", 0));
    EXPECT_EQ(128U, aconfig_unsigned(node.get(), "unsigned", 0));
    EXPECT_STREQ("This is a string", aconfig_str(node.get(), "string", "bah"));
}
