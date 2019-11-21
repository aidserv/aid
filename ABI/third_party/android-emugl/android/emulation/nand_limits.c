// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/emulation/nand_limits.h"

#include "android/utils/debug.h"

#include <signal.h>
#include <stdlib.h>
#include <string.h>

#define T(...) VERBOSE_PRINT(nand_limits, __VA_ARGS__)

static bool parse_nand_rw_limit(const char* value,
                                const char* value_end,
                                uint64_t* v) {
    char* end;
    uint64_t val = strtoul(value, &end, 0);

    if (end == value) {
        derror("bad parameter value '%s': expecting unsigned integer", value);
        return false;
    }

    switch (end[0]) {
        case 'K':
            val <<= 10;
            break;
        case 'M':
            val <<= 20;
            break;
        case 'G':
            val <<= 30;
            break;
        case 0:
        case ',':
            break;
        default:
            derror("bad read/write limit suffix: use K, M or G, got [%s]", end);
            return false;
    }
    *v = val;
    return true;
}

bool android_nand_limits_parse(const char* limits,
                               AndroidNandLimit* read_limit,
                               AndroidNandLimit* write_limit) {
    int pid = -1, signal = -1;
    uint64_t reads = 0, writes = 0;
    const char* item = limits;

#ifdef _WIN32
    // Currently not working on Windows.
    derror("-nand-limits is not available on Windows!");
    return false;
#endif

    /* parse over comma-separated items */
    while (item && *item) {
        const char* next_item;
        const char* comma = strchr(item, ',');
        char* end;

        if (!comma) {
            next_item = item + strlen(item);
            comma = next_item;
        } else {
            next_item = comma + 1;
        }

        if (!strncmp(item, "pid=", 4)) {
            pid = strtol(item + 4, &end, 10);
            if (end != comma) {
                derror("bad parameter, expecting pid=<number>, got '%s'", item);
                return false;
            }
            if (pid <= 0) {
                derror("bad parameter: process identifier must be > 0");
                return false;
            }
        } else if (!strncmp(item, "signal=", 7)) {
            signal = strtol(item + 7, &end, 10);
            if (end != comma) {
                derror("bad parameter: expecting signal=<number>, got '%s'",
                       item);
                return false;
            }
            if (signal <= 0) {
                derror("bad parameter: signal number must be > 0");
                return false;
            }
        } else if (!strncmp(item, "reads=", 6)) {
            if (!parse_nand_rw_limit(item + 6, next_item, &reads)) {
                return false;
            }
        } else if (!strncmp(item, "writes=", 7)) {
            if (!parse_nand_rw_limit(item + 7, next_item, &writes)) {
                return false;
            }
        } else {
            derror("bad parameter '%s' (see -help-nand-limits)", item);
            return false;
        }
        item = next_item;
    }
    if (pid < 0) {
        derror("bad paramater: missing pid=<number>");
        return false;
    } else if (signal < 0) {
        derror("bad parameter: missing signal=<number>");
        return false;
    } else if (reads == 0 && writes == 0) {
        dwarning("no read or write limit specified. ignoring -nand-limits");
        return false;
    }
    read_limit->pid = pid;
    read_limit->signal = signal;
    read_limit->counter = 0;
    read_limit->limit = reads;

    write_limit->pid = pid;
    write_limit->signal = signal;
    write_limit->counter = 0;
    write_limit->limit = writes;

    return true;
}

void android_nand_limit_update(AndroidNandLimit* l, uint32_t count) {
    if (l->counter >= l->limit) {
        return;
    }
    uint64_t avail = l->limit - l->counter;
    if (avail > count) {
        avail = count;
    }

    if (l->counter == 0) {
        T("%s: starting threshold counting to %lld", __FUNCTION__, l->limit);
    }
    l->counter += avail;
    if (l->counter >= l->limit) {
        /* threshold reach, send a signal to an external process */
        T("%s: sending signal %d to pid %d !", __FUNCTION__, l->signal, l->pid);
#ifndef _WIN32
        // Currently not working on Windows.
        kill(l->pid, l->signal);
#endif
    }
    return;
}
