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

#include "android/base/ArraySize.h"

#include <gtest/gtest.h>

#include <vector>

namespace android {
namespace base {

TEST(ArraySize, Sizes) {
    int array1[100];
    EXPECT_EQ(100U, arraySize(array1));
    EXPECT_EQ(100U, ARRAY_SIZE(array1));

    char array2[200];
    EXPECT_EQ(200U, arraySize(array2));
    EXPECT_EQ(200U, ARRAY_SIZE(array2));

    std::array<std::vector<bool>, 15> array3;
    EXPECT_EQ(15U, arraySize(array3));
    EXPECT_EQ(15U, ARRAY_SIZE(array3));
}

TEST(ArraySize, CompileTime) {
    static constexpr int arr[20] = {};
    static_assert(ARRAY_SIZE(arr) == 20U,
                  "Bad ARRAY_SIZE() result in compile time");
    static_assert(arraySize(arr) == 20U,
                  "Bad arraySize() result in compile time");

    static constexpr bool arr2[arraySize(arr)] = {};
    static_assert(arraySize(arr2) == 20U,
                  "Bad size of a new array declared with a result of "
                  "arraySize() call");
    static_assert(arraySize(arr) == ARRAY_SIZE(arr2),
                  "Macro and function are compatible");
}

}  // namespace base
}  // namespace android
