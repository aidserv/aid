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

#include "android/base/files/ScopedStdioFile.h"

#include <gtest/gtest.h>

namespace android {
namespace base {

// The path of a file that can always be opened for reading on any platform.
#ifdef _WIN32
static const char kNullFile[] = "NUL";
#else
static const char kNullFile[] = "/dev/null";
#endif

TEST(ScopedStdioFile, DefaultConstructor) {
    ScopedStdioFile f;
    EXPECT_FALSE(f.get());
}

TEST(ScopedStdioFile, Constructor) {
    ScopedStdioFile f(fopen(kNullFile, "rb"));
    EXPECT_TRUE(f.get());
}

TEST(ScopedStdioFile, Release) {
    FILE* handle = NULL;
    ScopedStdioFile f(fopen(kNullFile, "rb"));
    EXPECT_TRUE(f.get());
    handle = f.release();
    EXPECT_FALSE(f.get());
    EXPECT_TRUE(handle);
    ::fclose(handle);
}

TEST(ScopedStdioFile, Swap) {
    ScopedStdioFile f1;
    ScopedStdioFile f2(fopen(kNullFile, "rb"));
    EXPECT_FALSE(f1.get());
    EXPECT_TRUE(f2.get());
    FILE* fp = f2.get();
    std::swap(f1, f2);
    EXPECT_FALSE(f2.get());
    EXPECT_TRUE(f1.get());
    EXPECT_EQ(fp, f1.get());
}

}  // namespace base
}  // namespace android
