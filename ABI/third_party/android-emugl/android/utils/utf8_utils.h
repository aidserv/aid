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

// Simple C wrapper for android/base/misc/Utf8Utils.h

#include "android/utils/compiler.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

ANDROID_BEGIN_HEADER

bool android_utf8_is_valid(const char* text, size_t text_len);

// Try to decode an UTF-8 encoded Unicode codepoint from the |text_len| bytes
// at |text|. On success, return the encoded length, and sets |*codepoint| to
// the corresponding Unicode value. On failure (invalid input), return -1.
int android_utf8_decode(const uint8_t* text,
                        size_t text_len,
                        uint32_t* codepoint);

// Try to encode a Unicode |codepoint| into its UTF-8 representation into
// |buffer|, which must hold at least |buffer_len| bytes.
// Return the size of the encoding in bytes, or -1 in case of error
// (e.g. invalid value).
int android_utf8_encode(uint32_t codepoint,
                        uint8_t* buffer,
                        size_t buffer_len);

ANDROID_END_HEADER
