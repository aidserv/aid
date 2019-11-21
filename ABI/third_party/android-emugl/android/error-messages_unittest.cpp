// Copyright (C) 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/error-messages.h"

#include <gtest/gtest.h>

#include <limits>

// This function is not intended for use in the application itself but it does
// exist. We're just going to declare it here to make unit tests more
// repeatable and reliable
extern "C" void android_init_error_clear();

namespace android {

static const struct {
    int errorCode;
    const char* errorMessage;
    const char* expectedMessage;
} kTestCases[] = {
    { 42, "Is that it?", "Is that it?" },
    { 0, "Zero", "Zero" },
    { 13, "", kUnknownInitError },
    { -7, "Negative values too!", "Negative values too!" },
    { std::numeric_limits<int>::max(), "A large number", "A large number" },
    { std::numeric_limits<int>::min(), "And a small one", "And a small one" },
    { -1000, nullptr, kUnknownInitError },
};

TEST(ErrorMessages, noErrorOccurredYet) {
    android_init_error_clear();
    EXPECT_FALSE(android_init_error_occurred());
    EXPECT_EQ(nullptr, android_init_error_get_message());
}

TEST(ErrorMessages, initErrorClearedAndOccurred) {
    for (const auto& test : kTestCases) {
        android_init_error_clear();
        android_init_error_set(test.errorCode, test.errorMessage);
        EXPECT_TRUE(android_init_error_occurred());
    }
}

TEST(ErrorMessages, getInitErrorCode) {
    for (const auto& test : kTestCases) {
        android_init_error_clear();
        android_init_error_set(test.errorCode, test.errorMessage);
        EXPECT_EQ(test.errorCode, android_init_error_get_error_code());
    }
}

TEST(ErrorMessages, getInitErrorMessage) {
    for (const auto& test : kTestCases) {
        android_init_error_clear();
        android_init_error_set(test.errorCode, test.errorMessage);
        EXPECT_STREQ(test.expectedMessage, android_init_error_get_message());
        // Expect that the string pointers are not the same, the message should
        // have been copied when setting it.
        EXPECT_NE(test.errorMessage, android_init_error_get_message());
    }
}

}  // namespace android
