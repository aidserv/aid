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

#include "android/emulation/qemud/android_qemud_common.h"
#include "android/emulation/qemud/android_qemud_sink.h"
#include "android/emulation/serial_line.h"
#include "android/utils/compiler.h"
#include "android/utils/system.h"

ANDROID_BEGIN_HEADER

/** HANDLING SERIAL PORT CONNECTION
 **/

/* The QemudSerial object receives data from the serial port interface.
 * It parses the header to extract the channel id and payload length,
 * then the message itself.
 *
 * Incoming messages are sent to a generic receiver identified by
 * the 'recv_opaque' and 'recv_func' parameters to qemud_serial_init()
 *
 * It also provides qemud_serial_send() which can be used to send
 * messages back through the serial port.
 */

/* max serial MTU. Don't change this without modifying
 * development/emulator/qemud/qemud.c as well.
 */
#define  MAX_SERIAL_PAYLOAD        4000

#if SUPPORT_LEGACY_QEMUD
typedef enum {
    QEMUD_VERSION_UNKNOWN,
    QEMUD_VERSION_LEGACY,
    QEMUD_VERSION_NORMAL
} QemudVersion;

#  define  LEGACY_LENGTH_OFFSET   0
#  define  LEGACY_CHANNEL_OFFSET  4
#endif

/* out of convenience, the incoming message is zero-terminated
 * and can be modified by the receiver (e.g. for tokenization).
 */
typedef void (* QemudSerialReceive)(void* opaque, int channel, uint8_t* msg, int msglen);

typedef struct QemudSerial {
    CSerialLine* sl;  /* serial line endpoint */

    /* managing incoming packets from the serial port */
    ABool need_header;
    int overflow;
    int in_size;
    int in_channel;
#if SUPPORT_LEGACY_QEMUD
    QemudVersion  version;
#endif
    QemudSink header[1];
    QemudSink payload[1];
    uint8_t data0[MAX_SERIAL_PAYLOAD + 1];

    /* receiver */
    QemudSerialReceive recv_func;
    /* receiver callback */
    void* recv_opaque;  /* receiver user-specific data */
} QemudSerial;

/* Save the state of a QemudSerial to a snapshot file.
 */
extern void qemud_serial_save(Stream* f, QemudSerial* s);

/* Load the state of a QemudSerial from a snapshot file.
 */
extern int qemud_serial_load(Stream* f, QemudSerial* s);

/* called by the charpipe to see how much bytes can be
 * read from the serial port.
 */
#if SUPPORT_LEGACY_QEMUD
extern void qemud_serial_send_legacy_probe( QemudSerial*  s );
#endif /* SUPPORT_LEGACY_QEMUD */

/* intialize a QemudSerial object with a charpipe endpoint
 * and a receiver.
 */
extern void qemud_serial_init(QemudSerial* s,
                              CSerialLine* sl,
                              QemudSerialReceive recv_func,
                              void* recv_opaque);

/* send a message to the serial port. This will add the necessary
 * header.
 */
extern void qemud_serial_send(QemudSerial* s,
                              int channel,
                              ABool framing,
                              const uint8_t* msg,
                              int msglen);

ANDROID_END_HEADER
