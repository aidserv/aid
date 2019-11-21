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

#include <stddef.h>
#include <stdint.h>

namespace android {
namespace base {

// Return true iff |text| corresponds to |textLen| bytes of valid UTF-8
// encoded text.
bool utf8IsValid(const char* text, size_t textLen);

// Decode a single UTF-8 encoded input value. Reads at most |textLen| bytes
// from |text| and tries to interpret the data as the start of an UTF-8
// encoded Unicode value. On success, return the length in bytes of the
// encoding and sets |*codepoint| to the corresponding codepoint.
// On error (e.g. invalid encoding, or input too short), return -1.
int utf8Decode(const uint8_t* text, size_t textLen, uint32_t* codepoint);

// Encode Unicode |codepoint| into its UTF-8 representation into |buffer|
// which must be at least |buffer_len| bytes. Returns encoding length, or
// -1 in case of error. Note that |buffer| can be NULL, in which case
// |buffer_len| is ignored and the full encoding length is returned.
int utf8Encode(uint32_t codepoint, uint8_t* buffer, size_t buffer_len);

}  // namespace base
}  // namespace android
