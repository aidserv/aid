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

#include "android/base/misc/Utf8Utils.h"

namespace android {
namespace base {

bool utf8IsValid(const char* text, size_t textLen) {
    const char* end = text + textLen;

    while (text < end) {
        size_t n = 0;
        int ch = *text;
        if (!(ch & 0x80)) {
            n = 0;
        } else if ((ch & 0xe0) == 0xc0) {
            n = 1;
        } else if ((ch & 0xf0) == 0xe0) {
            n = 2;
        } else if ((ch & 0xf8) == 0xf0) {
            n = 3;
        } else if ((ch & 0xfc) == 0xf8) {
            n = 4;
        } else if ((ch & 0xfe) == 0xfc) {
            n = 5;
        } else {
            return false;
        }
        if ((size_t)(end - text) < n) {
            return false;
        }
        for (text++; n > 0; text++, n--) {
            if ((*text & 0xc0) != 0x80) {
                return false;
            }
        }
    }
    return true;
}

int utf8Decode(const uint8_t* text, size_t textLen, uint32_t* codepoint) {
    if (!textLen) {
        // No input!
        *codepoint = 0;
        return 0;
    }

    int n = 0;
    int ch = *text;
    if (!(ch & 0x80)) {
        n = 0;
    } else if ((ch & 0xe0) == 0xc0) {
        n = 1;
    } else if ((ch & 0xf0) == 0xe0) {
        n = 2;
    } else if ((ch & 0xf8) == 0xf0) {
        n = 3;
    } else if ((ch & 0xfc) == 0xf8) {
        n = 4;
    } else if ((ch & 0xfe) == 0xfc) {
        n = 5;
    } else {
        // Invalid input.
        return -1;
    }
    if ((size_t)n + 1U > textLen) {
        // Input is too short.
        return -1;
    }
    
    uint32_t value = 0;
    if (n == 0) {
        value = text[0];
    } else {
        value = text[0] & (0x7f >> n);
        for (int i = 0; i < n; i++) {
            value = (value << 6U) | (uint32_t)(text[i + 1] & 0x3f);
        }
    }
    *codepoint = value;
    return n + 1;
}

int utf8Encode(uint32_t codepoint, uint8_t* buffer, size_t bufferLen) {
    int len = 0;

    if (codepoint <= 0x7f) {
        len = 1;
    } else if (codepoint <= 0x7ff) {
        len = 2;
    } else if (codepoint <= 0xffff) {
        len = 3;
    } else if (codepoint <= 0x1fffff) {
        len = 4;
    } else if (codepoint <= 0x3ffffff) {
        len = 5;
    } else if (codepoint <= 0x7fffffff) {
        len = 6;
    } else {
        // Invalid value.
        return -1;
    }
    if (!buffer) {
        return len;
    }
    if (bufferLen < (size_t)len) {
        // buffer too small.
        return -1;
    }

    for (int n = len - 1; n > 0; --n) {
        buffer[n] = (uint8_t)(codepoint & 0x3f) | 0x80;
        codepoint >>= 6;
    }

    if (len == 1) {
        buffer[0] = (uint8_t)codepoint;
    } else {
        buffer[0] = (uint8_t)(0xff00 >> len) | codepoint;
    }

    return len;
}

}  // namespace base
}  // namespace android
