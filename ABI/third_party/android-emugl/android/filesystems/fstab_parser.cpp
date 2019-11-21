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

#include "android/filesystems/fstab_parser.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

namespace {

const char* skipWhitespace(const char* p, const char* end) {
    while (p < end && isspace(*p)) {
        p++;
    }
    return p;
}

const char* skipNonWhitespace(const char* p, const char* end) {
    while (p < end && !isspace(*p)) {
        p++;
    }
    return p;
}

const char* skipExpectedToken(const char* p, const char* end) {
    p = skipWhitespace(p, end);
    if (p == end) {
        return NULL;
    }
    return skipNonWhitespace(p, end);
}

size_t getTokenLen(const char* p, const char* end) {
    const char* q = skipNonWhitespace(p, end);
    return (size_t)(q - p);
}

}  // namespace

bool android_parseFstabPartitionFormat(const char* fstabData,
                                       size_t fstabSize,
                                       const char* partitionName,
                                       char** out) {
    const char* p = fstabData;
    const char* end = p + fstabSize;

    size_t partitionNameLen = strlen(partitionName);

    while (p < end) {
        // Find end of current line, and start of next one.
        const char* line = p;
        const char* line_end = ::strchr(p, '\n');
        if (!line_end) {
            line_end = end;
            p = end;
        } else {
            p = line_end + 1;
        }

        // Skip empty or comment lines.
        line = skipWhitespace(line, line_end);
        if (line == line_end || line[0] == '#') {
            continue;
        }

        // expected format: <device><ws><partition><ws><format><ws><options>

        // skip over device name.
        line = skipExpectedToken(line, line_end);
        if (!line) {
            continue;
        }

        line = skipWhitespace(line, line_end);
        size_t tokenLen = getTokenLen(line, line_end);
        if (tokenLen != partitionNameLen ||
            memcmp(line, partitionName, tokenLen) != 0) {
            // Not the right partition.
            continue;
        }

        line = skipWhitespace(line + tokenLen, line_end);
        size_t formatLen = getTokenLen(line, line_end);
        if (formatLen == 0) {
            // Malformed data.
            return false;
        }

        *out = static_cast<char*>(malloc(formatLen + 1U));
        memcpy(*out, line, formatLen);
        (*out)[formatLen] = '\0';
        return true;
    }
    return false;
}
