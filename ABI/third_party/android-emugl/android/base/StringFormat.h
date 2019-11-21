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

#pragma once

#include "android/base/StringView.h"

#include <string>
#include <type_traits>
#include <utility>

#include <stdarg.h>

namespace android {
namespace base {

// Create a new string instance that contains the printf-style formatted
// output from |format| and potentially any following arguments.
std::string StringFormatRaw(const char* format, ...);

// A variant of StringFormat() which uses a va_list to list formatting
// parameters instead.
std::string StringFormatWithArgs(const char* format, va_list args);

// Appends a formatted string at the end of an existing string.
// |string| is the target string instance, |format| the format string,
// followed by any formatting parameters. This is more efficient than
// appending the result of StringFormat(format,...) to |*string| directly.
void StringAppendFormatRaw(std::string* string, const char* format, ...);

// A variant of StringAppendFormat() that takes a va_list to list
// formatting parameters.
void StringAppendFormatWithArgs(std::string* string,
                                const char* format,
                                va_list args);

// unpackFormatArg() is a set of overloaded functions needed to unpack
// an argument of the formatting list to a POD value which can be passed
// into the sprintf()-like C function

// Anything which can be used to construct a string view goes here and unpacks
// into a const char*
constexpr const char* unpackFormatArg(StringView str) {
    return str.str();
}
// Forward all PODs as-is
template <class T>
constexpr T&& unpackFormatArg(T&& t,
        typename std::enable_if<
                    std::is_pod<typename std::decay<T>::type>::value
                 >::type* = nullptr) {
    return std::forward<T>(t);
}

// These templated versions of StringFormat*() allow one to pass all kinds of
// string objects into the argument list
template <class... Args>
std::string StringFormat(const char* format, Args&&... args) {
    return StringFormatRaw(format, unpackFormatArg(std::forward<Args>(args))...);
}

template <class... Args>
void StringAppendFormat(std::string* string,
                        const char* format,
                        Args&&... args) {
    StringAppendFormatRaw(string, format,
                          unpackFormatArg(std::forward<Args>(args))...);
}

}  // namespace base
}  // namespace android
