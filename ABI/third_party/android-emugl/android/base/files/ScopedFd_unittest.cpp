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

#include "android/base/files/ScopedFd.h"

#include <unistd.h>
#include <fcntl.h>

#include <gtest/gtest.h>

namespace android {
namespace base {

namespace {

// The path of a file that can always be opened for reading on any platform.
#ifdef _WIN32
static const char kNullFile[] = "NUL";
#else
static const char kNullFile[] = "/dev/null";
#endif

int OpenNull() {
    return ::open(kNullFile, O_RDONLY);
}

}  // namespace

TEST(ScopedFd, DefaultConstructor) {
    ScopedFd f;
    EXPECT_FALSE(f.valid());
    EXPECT_EQ(-1, f.get());
}

TEST(ScopedFd, Constructor) {
    ScopedFd f(OpenNull());
    EXPECT_TRUE(f.valid());
}

TEST(ScopedFd, Release) {
    ScopedFd f(OpenNull());
    EXPECT_TRUE(f.valid());
    int fd = f.release();
    EXPECT_FALSE(f.valid());
    EXPECT_NE(-1, fd);
    ::close(fd);
}

TEST(ScopedFd, Close) {
    ScopedFd f(OpenNull());
    EXPECT_TRUE(f.valid());
    f.close();
    EXPECT_FALSE(f.valid());
}

TEST(ScopedFd, Swap) {
    ScopedFd f1;
    ScopedFd f2(OpenNull());
    EXPECT_FALSE(f1.valid());
    EXPECT_TRUE(f2.valid());
    int fd = f2.get();
    f1.swap(&f2);
    EXPECT_FALSE(f2.valid());
    EXPECT_TRUE(f1.valid());
    EXPECT_EQ(fd, f1.get());
}


}  // namespace base
}  // namespace android
