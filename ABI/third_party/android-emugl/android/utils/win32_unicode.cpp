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

#include "android/utils/win32_unicode.h"

#include "android/base/system/Win32UnicodeString.h"

using android::base::Win32UnicodeString;

extern "C" {

char* win32_utf16_to_utf8_str(const wchar_t* str) {
    auto narrow = Win32UnicodeString::convertToUtf8(str);
    if (narrow.empty()) {
        return nullptr;
    }
    return strdup(narrow.c_str());
}

int win32_utf16_to_utf8_buf(const wchar_t* str, char* buffer, int size) {
    int written = Win32UnicodeString::convertToUtf8(buffer, size, str);
    if (written < 0 || written >= size) {
        return -1;
    }
    return written;
}

wchar_t* win32_utf8_to_utf16_str(const char* str) {
    Win32UnicodeString wide(str);
    return wide.release();
}

}  // extern "C"

