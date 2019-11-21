/* Copyright (C) 2010-2015 The Android Open Source Project
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

#include "android/skin/keycode-buffer.h"

#include "android/utils/debug.h"

#include <stdio.h>

void skin_keycode_buffer_init(SkinKeycodeBuffer* buffer,
                              SkinKeyCodeFlushFunc flush_func) {
    buffer->keycode_flush = flush_func;
    buffer->keycode_count = 0;
}

void skin_keycode_buffer_add(SkinKeycodeBuffer* keycodes,
                              unsigned code,
                              bool down) {
    if (code != 0 && keycodes->keycode_count < MAX_KEYCODES) {
        keycodes->keycodes[(int)keycodes->keycode_count++] =
                ( (code & 0x3ff) | (down ? 0x400 : 0) );
    }
}

void skin_keycode_buffer_flush(SkinKeycodeBuffer* keycodes) {
    if (keycodes->keycode_count > 0) {
        if (VERBOSE_CHECK(keys)) {
            int  nn;
            printf(">> KEY");
            for (nn = 0; nn < keycodes->keycode_count; nn++) {
                int  code = keycodes->keycodes[nn];
                printf(" [0x%03x,%s]",
                       (code & 0x3ff), (code & 0x400) ? "down" : " up ");
            }
            printf("\n");
        }
        if (keycodes->keycode_flush) {
            keycodes->keycode_flush(keycodes->keycodes,
                                    keycodes->keycode_count);
        }
        keycodes->keycode_count = 0;
    }
}
