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

#include "android/utils/format.h"

#include <algorithm>
#include <ctype.h>
#include <stdint.h>

// append a character if there is room for it (and a terminating null)
// always increment the result
#define DST_APPEND_SAFE(c) if (dstPos+1 < dstLen) { dst[dstPos++] = c; } result++;

size_t format_hex(char* dst, size_t dstLen, const void* srcVoid, size_t srcLen) {
    size_t dstPos = 0;
    size_t result = 0;

    // insert a space between every kSpaceInterval src bytes
    static const int kSpaceInterval = 4;

    const static char digitMap[] = "0123456789abcdef";
    const uint8_t* src = (const uint8_t*)srcVoid;
    for (size_t srcPos = 0; srcPos < srcLen; ++srcPos) {
        if (srcPos > 0 && (srcPos % kSpaceInterval) == 0) {
            // time to append a space
            DST_APPEND_SAFE(' ');
        }
        uint8_t b = src[srcPos];
        char hi = digitMap[b>>4];
        char lo = digitMap[b&0xf];
        DST_APPEND_SAFE(hi);
        DST_APPEND_SAFE(lo);
    }
    // null-terminate if possible
    if (dstLen > 0) {
        dst[dstPos] = 0;
    }
    return result;
}

size_t format_printable(char* dst, size_t dstLen, const void* srcVoid, size_t srcLen) {
    size_t dstPos = 0;
    size_t result = 0;

    // insert a space between every kSpaceInterval src bytes
    static const int kSpaceInterval = 8;

    const uint8_t* src = (const uint8_t*)srcVoid;
    for (size_t srcPos = 0; srcPos < srcLen; ++srcPos) {
        if (srcPos > 0 && (srcPos % kSpaceInterval) == 0) {
            // time to append a space
            DST_APPEND_SAFE(' ');
        }
        uint8_t b = src[srcPos];
        char out = isprint(b) ? (char)b : '.';
        DST_APPEND_SAFE(out);
    }

    // null-terminate if possible
    if (dstLen > 0) {
        dst[dstPos] = 0;
    }
    return result;
}

size_t format_hex_printable(char* dst, size_t dstLen, const void* src, size_t srcLen) {
    size_t result = format_hex(dst, dstLen, src, srcLen);

    size_t dstPos = (dstLen == 0) ? 0 : std::min(result,dstLen-1);

    DST_APPEND_SAFE(' ');
    DST_APPEND_SAFE(' ');
    DST_APPEND_SAFE(' ');
    DST_APPEND_SAFE(' ');
    // null-terminate if possible
    if (dstLen > 0) {
        dst[dstPos] = 0;
    }

    result += format_printable(dst+dstPos, dstLen-dstPos, src, srcLen);

    return result;
}

char* format_hex_printable2(char* dst, size_t dstLen, const void* src, size_t srcLen) {
    format_hex_printable(dst, dstLen, src, srcLen);
    return dst;
}

