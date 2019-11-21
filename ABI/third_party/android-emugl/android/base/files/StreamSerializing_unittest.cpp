// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/files/StreamSerializing.h"

#include "android/base/files/MemStream.h"
#include "android/base/testing/GTestUtils.h"

#include <gtest/gtest.h>

namespace android {
namespace base {

TEST(StreamSerializing, vector) {
    const std::vector<int> v = {1,2,3,4,5,6,200};
    MemStream saveStream;
    saveBuffer(&saveStream, v);

    std::vector<int> v2;
    loadBuffer(&saveStream, &v2);
    EXPECT_TRUE(RangesMatch(v, v2));
    EXPECT_EQ(0, saveStream.readSize());
}

TEST(StreamSerializing, smallVector) {
    const SmallFixedVector<short, 4> v = {1,2,3,4,5,6,200};
    MemStream saveStream;
    saveBuffer(&saveStream, v);

    SmallFixedVector<short, 40> v2;
    loadBuffer(&saveStream, &v2);
    EXPECT_TRUE(RangesMatch(v, v2));
    EXPECT_EQ(0, saveStream.readSize());
}

}  // namespace base
}  // namespace android
