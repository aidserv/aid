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

#include "android/base/sockets/ScopedSocket.h"
#include "android/base/sockets/SocketUtils.h"

#include <gtest/gtest.h>

#include <utility>

namespace android {
namespace base {

namespace {

int OpenNull() {
    return socketCreateTcp4();
}

}  // namespace

TEST(ScopedSocket, DefaultConstructor) {
    ScopedSocket f;
    EXPECT_FALSE(f.valid());
    EXPECT_EQ(-1, f.get());
}

TEST(ScopedSocket, Constructor) {
    ScopedSocket f(OpenNull());
    EXPECT_TRUE(f.valid());

    ScopedSocket f2(-1);
    EXPECT_FALSE(f2.valid());
}

TEST(ScopedSocket, Release) {
    ScopedSocket f(OpenNull());
    EXPECT_TRUE(f.valid());
    int fd = f.release();
    EXPECT_FALSE(f.valid());
    EXPECT_NE(-1, fd);
    socketClose(fd);
}

TEST(ScopedSocket, Reset) {
    ScopedSocket f(OpenNull());
    int fd2 = OpenNull();
    EXPECT_TRUE(f.valid());
    f.reset(fd2);
    EXPECT_TRUE(f.valid());
    EXPECT_EQ(fd2, f.get());
}

TEST(ScopedSocket, Close) {
    ScopedSocket f(OpenNull());
    EXPECT_TRUE(f.valid());
    f.close();
    EXPECT_FALSE(f.valid());
}

TEST(ScopedSocket, Swap) {
    ScopedSocket f1;
    ScopedSocket f2(OpenNull());
    EXPECT_FALSE(f1.valid());
    EXPECT_TRUE(f2.valid());
    f1.swap(&f2);
    EXPECT_FALSE(f2.valid());
    EXPECT_TRUE(f1.valid());
    swap(f1, f2);
    EXPECT_FALSE(f1.valid());
    EXPECT_TRUE(f2.valid());
}

TEST(ScopedSocket, Move) {
    ScopedSocket f1(OpenNull());
    ScopedSocket f2(std::move(f1));
    EXPECT_FALSE(f1.valid());
    EXPECT_TRUE(f2.valid());
    f1 = std::move(f2);
    EXPECT_FALSE(f2.valid());
    EXPECT_TRUE(f1.valid());
}

}  // namespace base
}  // namespace android
