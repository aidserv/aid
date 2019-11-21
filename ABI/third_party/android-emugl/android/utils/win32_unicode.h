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

ANDROID_BEGIN_HEADER

// Convert a null-terminated string |str| from UTF-16 to UTF-8 and return a
// pointer to the converted string. The function allocates this memory to match
// the required size and it's the callers responsibility to free it. Returns
// NULL if the conversion fails.
char* win32_utf16_to_utf8_str(const wchar_t* str);

// Convert a null-terminated string |str| from UTF-16 to UTF-8 and place the
// result in |buffer|. A maximum of |size| characters in |buffer| will be used.
// If the conversion fails or if there is not enough space in |buffer| as
// indicated by |size| the function returns -1. On success the number of
// characters written to |buffer| is returned.
int win32_utf16_to_utf8_buf(const wchar_t* str, char* buffer, int size);

// Convert a null-terminated string |str| from UTF-8 to UTF-16 and return a
// pointer to the converted string. The function allocates this memory to match
// the required size and it's the callers responsibility to free it. Returns
// NULL if the conversion fails.
wchar_t* win32_utf8_to_utf16_str(const char* str);

ANDROID_END_HEADER

