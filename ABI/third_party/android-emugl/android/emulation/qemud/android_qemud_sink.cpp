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

#include "android/emulation/qemud/android_qemud_sink.h"

#include <string.h>

void qemud_sink_save(Stream* f, const QemudSink* s) {
    stream_put_be32(f, s->used);
    stream_put_be32(f, s->size);
}

int qemud_sink_load(Stream* f, QemudSink* s) {
    s->used = stream_get_be32(f);
    s->size = stream_get_be32(f);
    return 0;
}

void qemud_sink_reset(QemudSink* ss, int size, uint8_t* buffer) {
    ss->used = 0;
    ss->size = size;
    ss->buff = buffer;
}

int qemud_sink_fill(QemudSink* ss, const uint8_t** pmsg, int* plen) {
    int avail = ss->size - ss->used;

    if (avail <= 0)
        return 1;

    if (avail > *plen)
        avail = *plen;

    memcpy(ss->buff + ss->used, *pmsg, avail);
    *pmsg += avail;
    *plen -= avail;
    ss->used += avail;

    return (ss->used == ss->size);
}

int qemud_sink_needed(const QemudSink* ss) {
    return ss->size - ss->used;
}
