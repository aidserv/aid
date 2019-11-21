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

#pragma once

// This file defines macroes ARRAY_SIZE() and STRING_LITERAL_LENGTH for static
// array size and string literal length detection
// If used from C, it's just a standard division of sizes. In C++ though, it
// would give you a compile-time error if used with something other than
// a built-in array or std::array<>
// Also, C++ defines corresponding constexpr functions for the same purpose

#ifndef __cplusplus
#   define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#   define STRING_LITERAL_LENGTH(str) (ARRAY_SIZE(str) - 1))
#else

#include <array>

namespace android {
namespace base {

template <class T, size_t N>
static constexpr size_t arraySize(const T (&arr)[N]) {
    return N;
}

template <class T, size_t N>
static constexpr size_t arraySize(const std::array<T, N>& arr) {
    return N;
}

template <size_t N>
static constexpr size_t stringLiteralLength(const char (&str)[N]) {
    return N - 1;
}

}  // namespace base
}  // namespace android

// for those who like macros, define it to be a simple function call
#define ARRAY_SIZE(arr) (::android::base::arraySize(arr))
#define STRING_LITERAL_LENGTH(str) (::android::base::stringLiteralLength(str))

#endif  // __cplusplus
