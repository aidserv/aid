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

#include "android/utils/property_file.h"

#include "android/utils/system.h"

#include <string.h>
#include <stdlib.h>

// Return true iff |ch| is whitespace. Don't use isspace() to avoid
// locale-related results and associated issues.
static int isspace(int ch) {
    return (ch == ' ' || ch == '\t');
}

void propertyFileIterator_init(PropertyFileIterator* iter,
                               const void* propFile,
                               size_t propFileLen) {
    iter->name[0] = '\0';
    iter->value[0] = '\0';
    iter->p = propFile;
    iter->end = iter->p + propFileLen;
}


bool propertyFileIterator_next(PropertyFileIterator* iter) {
    const char* p = iter->p;
    const char* end = iter->end;
    while (p < end) {
        // Get end of line, and compute next line position.
        const char* line = p;
        const char* lineEnd = (const char*)memchr(p, '\n', end - p);
        if (!lineEnd) {
            lineEnd = end;
            p = end;
        } else {
            p = lineEnd + 1;
        }

        // Remove trailing \r before the \n, if any.
        if (lineEnd > line && lineEnd[-1] == '\r')
            lineEnd--;

        // Skip leading whitespace.
        while (line < lineEnd && isspace(line[0]))
            line++;

        // Skip empty lines, and those that begin with '#' for comments.
        if (lineEnd == line || line[0] == '#')
            continue;

        const char* name = line;
        const char* nameEnd =
                (const char*)memchr(name, '=', lineEnd - name);
        if (!nameEnd) {
            // Skipping lines without a =
            continue;
        }
        const char* value = nameEnd + 1;
        while (nameEnd > name && isspace(nameEnd[-1]))
            nameEnd--;

        size_t nameLen = nameEnd - name;
        if (nameLen == 0 || nameLen >= MAX_PROPERTY_NAME_LEN) {
            // Skip lines without names, or with names too long.
            continue;
        }

        memcpy(iter->name, name, nameLen);
        iter->name[nameLen] = '\0';

        // Truncate value's length.
        size_t valueLen = (lineEnd - value);
        if (valueLen >= MAX_PROPERTY_VALUE_LEN)
            valueLen = (MAX_PROPERTY_VALUE_LEN - 1);

        memcpy(iter->value, value, valueLen);
        iter->value[valueLen] = '\0';

        iter->p = p;
        return true;
    }
    iter->p = p;
    return false;
}

char* propertyFile_getValue(const char* propFile,
                            size_t propFileLen,
                            const char* propName) {
    size_t propNameLen = strlen(propName);
    if (propNameLen >= MAX_PROPERTY_NAME_LEN)
        return NULL;

    char* ret = NULL;
    PropertyFileIterator iter[1];
    propertyFileIterator_init(iter, propFile, propFileLen);
    while (propertyFileIterator_next(iter)) {
        if (!strcmp(iter->name, propName)) {
            free(ret);
            ret = ASTRDUP(iter->value);
        }
    }
    return ret;
}
