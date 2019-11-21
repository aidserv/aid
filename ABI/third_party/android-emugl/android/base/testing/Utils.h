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

#include "android/base/memory/ScopedPtr.h"
#include "android/base/system/System.h"

#include <gtest/gtest.h>

#include <locale.h>

#define SKIP_TEST_ON_WINE()                        \
    do {                                           \
        if (System::get()->isRunningUnderWine()) { \
            return;                                \
        }                                          \
    } while (0)

inline auto setScopedCommaLocale() ->
    android::base::ScopedCustomPtr<char, void(*)(const char*)> {
#ifdef _WIN32
    static constexpr char commaLocaleName[] = "French";
#else
    static constexpr char commaLocaleName[] = "fr_FR.UTF-8";
#endif

    // Set up a locale with comma as a decimal mark.
    auto oldLocale = setlocale(LC_ALL, nullptr);
    EXPECT_NE(nullptr, oldLocale);
    if (!oldLocale) {
        return {};
    }
    // setlocale() will overwrite the |oldLocale| pointer's data, so copy it.
    auto oldLocaleCopy = android::base::ScopedCPtr<char>(strdup(oldLocale));
    auto newLocale = setlocale(LC_ALL, commaLocaleName);
    EXPECT_NE(nullptr, newLocale);
    if (!newLocale) {
        return {};
    }

    EXPECT_STREQ(",", localeconv()->decimal_point);

    // Restore the locale after the test.
    return android::base::makeCustomScopedPtr(
            oldLocaleCopy.release(), [](const char* name) {
                auto nameDeleter = android::base::ScopedCPtr<const char>(name);
                EXPECT_NE(nullptr, setlocale(LC_ALL, name));
            });
}
