/* Copyright (C) 2007-2008 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/
#include "android/utils/dirscanner.h"

#include "android/base/system/System.h"
#include "android/base/files/PathUtils.h"

#include <string>
#include <vector>

using android::base::PathUtils;

struct DirScanner {
    std::vector<std::string> entries;
    std::string prefix;
    std::string result;
    size_t pos;

    explicit DirScanner(const char* dir) :
            entries(),
            prefix(dir),
            result(),
            pos(0u) {
        entries = android::base::System::get()->scanDirEntries(dir);
        // Append directory separator if needed.
        prefix = PathUtils::addTrailingDirSeparator(prefix);
    }
};

const char* dirScanner_next(DirScanner* s) {
    if (s->pos < s->entries.size()) {
        return s->entries[s->pos++].c_str();
    }
    return NULL;
}

const char* dirScanner_nextFull(DirScanner* s) {
    if (s->pos < s->entries.size()) {
        s->result = s->prefix;
        s->result += s->entries[s->pos++];
        return s->result.c_str();
    }
    return NULL;
}

size_t dirScanner_numEntries(DirScanner* s) {
    return s->entries.size();
}

DirScanner* dirScanner_new(const char* rootPath) {
    DirScanner* s = new DirScanner(rootPath);
    return s;
}

void dirScanner_free(DirScanner* s) {
    delete s;
}
