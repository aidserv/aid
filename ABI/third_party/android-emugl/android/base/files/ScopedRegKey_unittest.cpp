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

#include "android/base/files/ScopedRegKey.h"

#include <gtest/gtest.h>

namespace android {
namespace base {

namespace {

// The path of a file that can always be opened for reading on any platform.
static const char kNullFile[] = "NUL";

HKEY OpenHKLM(bool* ok) {
    HKEY hkey = 0;
    *ok = ERROR_SUCCESS == ::RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                          NULL,
                                          0,
                                          KEY_READ,
                                          &hkey);
    return hkey;
}

}  // namespace

TEST(ScopedRegKey, DefaultConstructor) {
    ScopedRegKey h;
    EXPECT_FALSE(h.valid());
}

TEST(ScopedRegKey, Constructor) {
    bool ok;
    ScopedRegKey h(OpenHKLM(&ok));
    EXPECT_TRUE(ok);
    EXPECT_TRUE(h.valid());
}

TEST(ScopedRegKey, Release) {
    bool ok;
    ScopedRegKey h(OpenHKLM(&ok));
    EXPECT_TRUE(ok);
    EXPECT_TRUE(h.valid());
    HKEY hkey = h.release();
    EXPECT_FALSE(h.valid());
    ::RegCloseKey(hkey);
}

TEST(ScopedRegKey, Close) {
    bool ok;
    ScopedRegKey h(OpenHKLM(&ok));
    EXPECT_TRUE(ok);
    EXPECT_TRUE(h.valid());
    h.close();
    EXPECT_FALSE(h.valid());
}

TEST(ScopedRegKey, Swap) {
    ScopedRegKey h1;
    bool ok;
    ScopedRegKey h2(OpenHKLM(&ok));
    EXPECT_TRUE(ok);
    EXPECT_FALSE(h1.valid());
    EXPECT_TRUE(h2.valid());
    HKEY hkey = h2.get();
    h1.swap(&h2);
    EXPECT_FALSE(h2.valid());
    EXPECT_TRUE(h1.valid());
    EXPECT_EQ(hkey, h1.get());
}

}  // namespace base
}  // namespace android
