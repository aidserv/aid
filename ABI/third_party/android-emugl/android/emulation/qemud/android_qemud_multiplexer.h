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

#include "android/emulation/android_qemud.h"
#include "android/emulation/qemud/android_qemud_client.h"
#include "android/emulation/qemud/android_qemud_serial.h"
#include "android/emulation/qemud/android_qemud_service.h"
#include "android/utils/compiler.h"

#include "android/base/synchronization/Lock.h"

ANDROID_BEGIN_HEADER

/* A QemudMultiplexer object maintains the global state of the
 * qemud service facility. It holds a QemudSerial object to
 * maintain the state of the serial port connection.
 *
 * The QemudMultiplexer receives all incoming messages from
 * the serial port, and dispatches them to the appropriate
 * QemudClient.
 *
 * It also has a global list of clients, and a global list of
 * services.
 *
 * Finally, the QemudMultiplexer has a special QemudClient used
 * to handle channel 0, i.e. the control channel used to handle
 * connections and disconnections of clients.
 */
typedef struct QemudMultiplexer {
    // TODO: Hide QemudMultiplexer struct
    // from the header/client.
    QemudSerial serial[1];
    QemudClient* clients;
    QemudService* services;
    android::base::Lock lock;
} QemudMultiplexer;

/* this is the serial_recv callback that is called
 * whenever an incoming message arrives through the serial port
 */
extern void qemud_multiplexer_serial_recv(void* opaque,
                                          int channel,
                                          uint8_t* msg,
                                          int msglen);

/* handle a new connection attempt. This returns 0 on
 * success, -1 if the service name is unknown, or -2
 * if the service's maximum number of clients has been
 * reached.
 */
extern int qemud_multiplexer_connect(QemudMultiplexer* m,
                                     const char* service_name,
                                     int channel_id);

/* disconnect a given client from its channel id */
extern void qemud_multiplexer_disconnect(QemudMultiplexer* m,
                                         int channel);

/* disconnects all channels, except for the control channel, without informing
 * the daemon in the guest that disconnection has occurred.
 *
 * Used to silently kill clients when restoring emulator state snapshots.
 */
extern void qemud_multiplexer_disconnect_noncontrol(QemudMultiplexer* m);

/* handle control messages. This is used as the receive
 * callback for the special QemudClient setup to manage
 * channel 0.
 *
 * note that the message is zero-terminated for convenience
 * (i.e. msg[msglen] is a valid memory read that returns '\0')
 */
extern void qemud_multiplexer_control_recv(void* opaque,
                                           uint8_t* msg,
                                           int msglen,
                                           QemudClient* client);

/* initialize the global QemudMultiplexer.
 */
extern void qemud_multiplexer_init(QemudMultiplexer* mult,
                                   CSerialLine* serial_line);


ANDROID_END_HEADER
