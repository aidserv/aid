/* Copyright 2014 The Android Open Source Project
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
#include "android/avd/scanner.h"

#include "android/avd/util.h"
#include "android/emulation/bufprint_config_dirs.h"
#include "android/utils/bufprint.h"
#include "android/utils/debug.h"
#include "android/utils/dirscanner.h"
#include "android/utils/path.h"
#include "android/utils/system.h"

#include <string.h>

#define D(...) VERBOSE_PRINT(init,__VA_ARGS__)

struct AvdScanner {
    DirScanner* scanner;
    char temp[PATH_MAX];
    char name[PATH_MAX];
};

AvdScanner* avdScanner_new(const char* sdk_home) {
    AvdScanner* s;

    ANEW0(s);

    char* p = s->temp;
    char* end = p + sizeof(s->temp);

    if (!sdk_home) {
        p = bufprint_avd_home_path(p, end);
    } else {
        p = bufprint(p, end, "%s" PATH_SEP ANDROID_AVD_DIR, sdk_home);
    }

    if (p >= end) {
        // Path is too long, no search will be performed.
        D("Path too long: %s\n", s->temp);
        return s;
    }
    if (!path_is_dir(s->temp)) {
        // Path does not exist, no search will be performed.
        D("Path is not a directory: %s\n", s->temp);
        return s;
    }
    s->scanner = dirScanner_new(s->temp);
    return s;
}

const char* avdScanner_next(AvdScanner* s) {
    if (s->scanner) {
        for (;;) {
            const char* entry = dirScanner_next(s->scanner);
            if (!entry) {
                // End of enumeration.
                break;
            }
            size_t entry_len = strlen(entry);
            if (entry_len < 4 || 
                memcmp(entry + entry_len - 4U, ".ini", 4) != 0) {
                // Can't possibly be a <name>.ini file.
                continue;
            }

            // It's a match, get rid of the .ini suffix and return it.
            entry_len -= 4U;
            if (entry_len >= sizeof(s->name)) {
                D("Name too long: %s\n", entry);
                continue;
            }
            memcpy(s->name, entry, entry_len);
            s->name[entry_len] = '\0';
            return s->name;
        }
    }
    return NULL;
}

void avdScanner_free(AvdScanner* s) {
    if (s) {
        if (s->scanner) {
            dirScanner_free(s->scanner);
            s->scanner = NULL;
        }
        AFREE(s);
    }
}
