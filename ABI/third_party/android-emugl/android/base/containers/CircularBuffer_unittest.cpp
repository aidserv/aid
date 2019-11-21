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

#include "android/base/containers/CircularBuffer.h"

#include <memory>
#include <gtest/gtest.h>

namespace android {
namespace base {

TEST(CircularBuffer, Empty) {
    CircularBuffer<int> cb(1);
    EXPECT_TRUE(cb.empty());
    EXPECT_EQ(0, cb.size());
}

TEST(CircularBuffer, AppendOnce) {
    CircularBuffer<int> cb(1);
    cb.push_back(10098);
    EXPECT_FALSE(cb.empty());
    EXPECT_EQ(1, cb.size());
    EXPECT_EQ(10098, cb[0]);
    EXPECT_EQ(10098, cb.front());
    EXPECT_EQ(10098, cb.back());
}

TEST(CircularBuffer, AppendTwice) {
    CircularBuffer<int> cb(1);
    cb.push_back(10098);
    EXPECT_FALSE(cb.empty());
    EXPECT_EQ(1, cb.size());
    EXPECT_EQ(10098, cb[0]);
    cb.push_back(10099);
    EXPECT_FALSE(cb.empty());
    EXPECT_EQ(1, cb.size());
    EXPECT_EQ(10099, cb[0]);
}

TEST(CircularBuffer, ManyItemsSimple) {
    CircularBuffer<int> cb(5);
    cb.push_back(1);
    cb.push_back(2);
    cb.push_back(3);
    EXPECT_EQ(1, cb.front());
    EXPECT_EQ(3, cb.back());
    EXPECT_EQ(3, cb.size());
    EXPECT_EQ(1, cb[0]);
    EXPECT_EQ(2, cb[1]);
    EXPECT_EQ(3, cb[2]);
}

TEST(CircularBuffer, ManyItems) {
    CircularBuffer<int> cb(5);
    cb.push_back(1);
    cb.push_back(2);
    cb.push_back(3);
    cb.push_back(4);
    cb.push_back(5);
    cb.push_back(6);
    cb.push_back(7);


    EXPECT_FALSE(cb.empty());
    EXPECT_EQ(5, cb.size());

    int expected_values[] = {3, 4, 5, 6, 7};
    EXPECT_EQ(3, cb.front());
    EXPECT_EQ(7, cb.back());
    for (int i = 0; i < cb.size(); ++i) {
        EXPECT_EQ(expected_values[i], cb[i]);
    }

    cb.pop_back();
    for (int i = 0; i < cb.size(); ++i) {
        EXPECT_EQ(expected_values[i], cb[i]);
    }
    EXPECT_EQ(3, cb.front());
    EXPECT_EQ(6, cb.back());
}

TEST(CircularBuffer, PopFront) {
    CircularBuffer<int> cb(1);
    cb.push_back(10098);
    EXPECT_FALSE(cb.empty());
    EXPECT_EQ(1, cb.size());
    EXPECT_EQ(10098, cb[0]);
    cb.pop_front();
    EXPECT_TRUE(cb.empty());
    EXPECT_EQ(0, cb.size());
}

TEST(CircularBuffer, PopBack) {
    CircularBuffer<int> cb(1);
    cb.push_back(10098);
    EXPECT_FALSE(cb.empty());
    EXPECT_EQ(1, cb.size());
    EXPECT_EQ(10098, cb[0]);
    cb.pop_back();
    EXPECT_TRUE(cb.empty());
    EXPECT_EQ(0, cb.size());
}

TEST(CircularBuffer, MoveOnlyType) {
   CircularBuffer<std::unique_ptr<int>> cb(1);
   std::unique_ptr<int> ptr(new int[1]);
   *ptr = 10098;
   cb.push_back(std::move(ptr));
   EXPECT_EQ(10098, *cb.back());
}

}
}

