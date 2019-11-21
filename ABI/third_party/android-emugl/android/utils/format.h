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

#include <stddef.h>

ANDROID_BEGIN_HEADER

// format_hex, format_printable, format_hex_printable,
// format_hex_printable2
//
// Hex format prints data into a character buffer
//
// |dst| character buffer to receive formatted text output is always
//       NUL-terminated if |dstLen| > 0
// |dstLen| maximum number of bytes to be written to |dst|
// |src| data to be printed
// |srcLen| number of bytes to be printed
//
// format_hex, format_printable, format_hex_printable:
// Returns length of the string the function tried to create
// (Like snprintf, strlcpy() and strlcat())
//
// format_hex_printable2:
// Returns |dst|

// Output is hex formatted with a space between every 4 src bytes
size_t format_hex(char* dst, size_t dstLen, const void* src, size_t srcLen);

// Output is ASCii formatted with a space between every 8 src bytes
// Unprintable characters are replaced with '.'
size_t format_printable(char* dst, size_t dstLen, const void* src, size_t srcLen);

// Output is hex formatted with a space between every 4 src bytes
// followed by 4 spaces followed by
// ASCii formatted with a space between every 8 src bytes
// Unprintable characters are replaced with '.'
size_t format_hex_printable(char* dst, size_t dstLen, const void* src, size_t srcLen);

// Output is same as format_hex_printable
char* format_hex_printable2(char* dst, size_t dstLen, const void* src, size_t srcLen);

ANDROID_END_HEADER
