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

#include "android/base/files/ScopedFileHandle.h"

#include <gtest/gtest.h>

namespace android {
namespace base {

namespace {

// The path of a file that can always be opened for reading on any platform.
static const char kNullFile[] = "NUL";

HANDLE OpenNull() {
    return ::CreateFile(kNullFile,
                        GENERIC_READ,
                        FILE_SHARE_READ|FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
}

}  // namespace

TEST(ScopedFileHandle, DefaultConstructor) {
    ScopedFileHandle h;
    EXPECT_FALSE(h.valid());
    EXPECT_EQ(INVALID_HANDLE_VALUE, h.get());
}

TEST(ScopedFileHandle, Constructor) {
    ScopedFileHandle h(OpenNull());
    EXPECT_TRUE(h.valid());
}

TEST(ScopedFileHandle, Release) {
    ScopedFileHandle h(OpenNull());
    EXPECT_TRUE(h.valid());
    HANDLE handle = h.release();
    EXPECT_FALSE(h.valid());
    EXPECT_NE(INVALID_HANDLE_VALUE, handle);
    ::CloseHandle(handle);
}

TEST(ScopedFileHandle, Close) {
    ScopedFileHandle h(OpenNull());
    EXPECT_TRUE(h.valid());
    h.close();
    EXPECT_FALSE(h.valid());
}

TEST(ScopedFileHandle, Swap) {
    ScopedFileHandle h1;
    ScopedFileHandle h2(OpenNull());
    EXPECT_FALSE(h1.valid());
    EXPECT_TRUE(h2.valid());
    HANDLE handle = h2.get();
    h1.swap(&h2);
    EXPECT_FALSE(h2.valid());
    EXPECT_TRUE(h1.valid());
    EXPECT_EQ(handle, h1.get());
}

}  // namespace base
}  // namespace android
