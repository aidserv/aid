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

#include "android/utils/string.h"

#include <algorithm>

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#if defined(__APPLE__) || defined(__FreeBSD__)

// already defined in C library

#else // __linux__ && _WIN32

// NOTE: linux has this defined in libbsd if we want to switch to
// that some day

size_t strlcpy(char* dst, const char * src, size_t size)
{
    size_t srcLen = strlen(src);
    if (size > 0) {
        size_t copyLen = std::min(srcLen, size-1);
        memcpy(dst, src, copyLen);
        dst[copyLen] = 0;
    }
    return srcLen;
}

#endif

bool str_begins_with(const char* string, const char* prefix) {
    return strncmp(prefix, string, strlen(prefix)) == 0;
}

bool str_ends_with(const char* str, const char* suffix) {
    int str_len = strlen(str);
    int suffix_len = strlen(suffix);
    return (str_len >= suffix_len) &&
           (0 == strcmp(str + (str_len - suffix_len), suffix));
}

const char* str_skip_white_space_if_any(const char* pos) {
    while (isspace(*pos)) {
        pos++;
    }
    return pos;
}

void str_reset(char** string, const char* new_value) {
    ::free(*string);
    *string = new_value ? ::strdup(new_value) : nullptr;
}

void str_reset_nocopy(char** string, char* new_value) {
    ::free(*string);
    *string = new_value;
}

void str_reset_null(char** string) {
    ::free(*string);
    *string = nullptr;
}
