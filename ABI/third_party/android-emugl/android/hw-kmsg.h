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

#pragma once

#include "android/emulation/serial_line.h"

#include <stdbool.h>

/* this chardriver is used to read the kernel messages coming
 * from the first serial port (i.e. /dev/ttyS0) and store them
 * in memory for later...
 */

typedef enum {
    ANDROID_KMSG_SAVE_MESSAGES  = (1 << 0),
    ANDROID_KMSG_PRINT_MESSAGES = (1 << 1),
} AndroidKmsgFlags;

extern bool  android_kmsg_init(AndroidKmsgFlags flags);

extern CSerialLine* android_kmsg_get_serial_line(void);
