// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// Some free functions for manipulating strings as URIs. Wherever possible,
// these functions take const references to StringView to avoid unnecessary
// copies.

#pragma once

#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

// This is a simple wrapper around android/base/Uri.h to expose the same
// functionality to C code.

// Returns percent encoded URI. Caller must free the returned string.
// Returns the empty string "" on encode error, NULL on memory allocation error.
extern char* uri_encode(const char* uri);

// Decodes a percent encoded URI. Caller must free the returned string.
// Returns the empty string "" on decode error, NULL on memory allocation error.
extern char* uri_decode(const char* uri);

ANDROID_END_HEADER
