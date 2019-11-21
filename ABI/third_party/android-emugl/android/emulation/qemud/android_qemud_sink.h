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

#pragma once

#include "android/utils/compiler.h"
#include "android/utils/stream.h"

ANDROID_BEGIN_HEADER

/* A QemudSink is just a handy data structure that is used to
 * read a fixed amount of bytes into a buffer
 */
typedef struct QemudSink {
    int used;
    /* number of bytes already used */
    int size;
    /* total number of bytes in buff */
    uint8_t* buff;
} QemudSink;


/* save the state of a QemudSink to a snapshot.
 *
 * The buffer pointer is not saved, since it usually points to buffer
 * fields in other structs, which have save functions themselves. It
 * is up to the caller to make sure the buffer is correctly saved and
 * restored.
 */
extern void qemud_sink_save(Stream* f, const QemudSink* s);

/* load the state of a QemudSink from a snapshot.
 */
extern int qemud_sink_load(Stream* f, QemudSink* s);

/* reset a QemudSink, i.e. provide a new destination buffer address
 * and its size in bytes.
 */
extern void qemud_sink_reset(QemudSink* ss, int size, uint8_t* buffer);

/* try to fill the sink by reading bytes from the source buffer
 * '*pmsg' which contains '*plen' bytes
 *
 * this functions updates '*pmsg' and '*plen', and returns
 * 1 if the sink's destination buffer is full, or 0 otherwise.
 */
extern int qemud_sink_fill(QemudSink* ss, const uint8_t** pmsg, int* plen);

/* returns the number of bytes needed to fill a sink's destination
 * buffer.
 */
extern int qemud_sink_needed(const QemudSink* ss);

ANDROID_END_HEADER
