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

#include "android/utils/utf8_utils.h"

#include "android/base/misc/Utf8Utils.h"

bool android_utf8_is_valid(const char* text, size_t text_len) {
    return android::base::utf8IsValid(text, text_len);
}

int android_utf8_decode(const uint8_t* text,
                        size_t text_len,
                        uint32_t* codepoint) {
    return android::base::utf8Decode(text, text_len, codepoint);
}

int android_utf8_encode(uint32_t codepoint,
                        uint8_t* buffer,
                        size_t buffer_len) {
    return android::base::utf8Encode(codepoint, buffer, buffer_len);
}
