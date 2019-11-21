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

#include "android/utils/compiler.h"
#include <stdarg.h>

//
// This file defines C and C++ replacements for scanf to parse a string in a
// locale-independent way. This is useful when parsing input data that comes
// not from user, but from some kind of a fixed protocol with predefined locale
// settings.
// Just use these functions as drop-in replacements of sscanf();
//
// Note1: if the input string contains any dot characters other than decimal
// separators, the results of parsing will be screwed: in Windows the
// implementation replaces all dots with the current decimal separator to parse
// using current locale.
// Note2: current implementation only supports parsing floating point numbers -
// no code for monetary values, dates, digit grouping etc.
// The limitation is because of MinGW's lack of per-thread locales support.
//

ANDROID_BEGIN_HEADER

int SscanfWithCLocale(const char* string, const char* format, ...);
int SscanfWithCLocaleWithArgs(const char* string, const char* format,
                              va_list args);

ANDROID_END_HEADER

#ifdef __cplusplus

#include <utility>

namespace android {
namespace base {

template <class... Args>
int SscanfWithCLocale(const char* string, const char* format, Args... args) {
    return ::SscanfWithCLocale(string, format, std::forward<Args>(args)...);
}

}  // namespace base
}  // namespace android

#endif  // __cplusplus
