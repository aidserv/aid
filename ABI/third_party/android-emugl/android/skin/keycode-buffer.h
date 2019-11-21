/* Copyright (C) 2010 The Android Open Source Project
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

#include "android/utils/compiler.h"

#include <stdbool.h>

ANDROID_BEGIN_HEADER

/* Contains declarations for routines that manage keycode sequence that needs
 * to be transferred to the emulator core for further processing.
 */

/* Maximum number of keycodes kept in the array. */
#define  MAX_KEYCODES   256*2

// Type of a function used to flush buffered codes
typedef void (*SkinKeyCodeFlushFunc)(int* codes, int count);

/* Describes array of keycodes collected for transferring to the core. */
typedef struct SkinKeycodeBuffer {
    SkinKeyCodeFlushFunc keycode_flush;
    int keycode_count;
    int keycodes[MAX_KEYCODES];
} SkinKeycodeBuffer;

void skin_keycode_buffer_init(SkinKeycodeBuffer* buffer,
                              SkinKeyCodeFlushFunc flush_func);

/* Adds a key event to the array of keycodes. */
void skin_keycode_buffer_add(SkinKeycodeBuffer* buffer,
                             unsigned code,
                             bool down);

/* Flushes (transfers) collected keycodes to the core. */
void skin_keycode_buffer_flush(SkinKeycodeBuffer* buffer);

ANDROID_END_HEADER
