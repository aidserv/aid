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
#include "android/hw-kmsg.h"

#include "android/emulation/serial_line.h"
#include "android/utils/debug.h"

#include <stdio.h>
#include <stdlib.h>

static CSerialLine* android_kmsg_serial_line;

typedef struct {
    CSerialLine* serial_line;
    AndroidKmsgFlags flags;
} KernelLog;

static int kernel_log_can_read(void* opaque) {
    return 1024;
}

static void kernel_log_read(void* opaque, const uint8_t* from, int len) {
    KernelLog* k = opaque;

    if (k->flags & ANDROID_KMSG_PRINT_MESSAGES) {
        printf("%.*s", len, (const char*)from);
    }

    /* XXXX: TODO: save messages into in-memory buffer for later retrieval */
}

static bool kernel_log_init(KernelLog* k, AndroidKmsgFlags flags) {
    if (android_serialline_pipe_open(&k->serial_line,
                                     &android_kmsg_serial_line) < 0) {
        derror( "could not create kernel log charpipe" );
        return false;
    }

    android_serialline_addhandlers(k->serial_line,
                                   k,
                                   kernel_log_can_read,
                                   kernel_log_read);
    k->flags = flags;
    return true;
}

static KernelLog  _kernel_log[1];

bool
android_kmsg_init(AndroidKmsgFlags flags) {
    if (_kernel_log->serial_line == NULL) {
        return kernel_log_init(_kernel_log, flags);
    }
    return true;
}


CSerialLine* android_kmsg_get_serial_line(void) {
    if (android_kmsg_serial_line == NULL) {
        android_kmsg_init(0);
    }
    return android_kmsg_serial_line;
}
