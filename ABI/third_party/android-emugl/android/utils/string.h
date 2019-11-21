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

#include "android/utils/compiler.h"
#include "android/utils/system.h"

#include <stdbool.h>
#include <stddef.h>
#if defined(__APPLE__) || defined(__FreeBSD__)
#include <string.h>
#endif

ANDROID_BEGIN_HEADER

#if !defined(__APPLE__) && !defined(__FreeBSD__)
size_t strlcpy(char* dst, const char * src, size_t size);
#endif

/* returns true if |string| ends with |suffix| */
bool str_ends_with(const char* string, const char* suffix);

/* returns true if |string| begins with |prefix| */
bool str_begins_with(const char* string, const char* prefix);

/* skips white space at pos if any, returns pointer to first
 * non-whitespace character
 */
const char* str_skip_white_space_if_any(const char* str);

// Reset the value of |*string| to a copy of |new_value|. This
// will free() the previous value of |*string| first. Note that
// |new_value| can be NULL if one wants to simply unset the string pointer.
void str_reset(char** string, const char* new_value);

// Same as str_reset() but do not copy |new_value|, instead transfer its
// ownership to |*string|. Note that |new_value| can be NULL.
void str_reset_nocopy(char** string, char* new_value);

// Same as str_reset(string, NULL), i.e. unset a string pointer.
void str_reset_null(char** string);

ANDROID_END_HEADER
