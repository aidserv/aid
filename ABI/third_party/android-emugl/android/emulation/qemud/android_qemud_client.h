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
#include "android/emulation/qemud/android_qemud_common.h"
#include "android/emulation/qemud/android_qemud_sink.h"
#include "android/utils/compiler.h"
#include "android/utils/system.h"

#include <stdbool.h>

ANDROID_BEGIN_HEADER


/* Descriptor for a data buffer pending to be sent to a qemud pipe client.
 *
 * When a service decides to send data to the client, there could be cases when
 * client is not ready to read them. In this case there is no AndroidPipeBuffer
 * available to write service's data to, So, we need to cache that data into the
 * client descriptor, and "send" them over to the client in _qemudPipe_recvBuffers
 * callback. Pending service data is stored in the client descriptor as a list
 * of QemudPipeMessage instances.
 */
typedef struct QemudPipeMessage QemudPipeMessage;
struct QemudPipeMessage {
    /* Message to send. */
    uint8_t* message;
    /* Message size. */
    size_t size;
    /* Offset in the message buffer of the chunk, that has not been sent
     * to the pipe yet. */
    size_t offset;
    /* Links next message in the client. */
    QemudPipeMessage* next;
};


/* A QemudClient models a single client as seen by the emulator.
 * Each client has its own channel id (for the serial qemud), or pipe descriptor
 * (for the pipe based qemud), and belongs to a given QemudService (see below).
 *
 * There is a global list of serial clients used to multiplex incoming
 * messages from the channel id (see qemud_multiplexer_serial_recv()). Pipe
 * clients don't need multiplexing, because they are communicated via qemud pipes
 * that are unique for each client.
 *
 */

/* Defines type of the client: pipe, or serial.
 */
typedef enum QemudProtocol {
    /* Client is communicating via pipe. */
    QEMUD_PROTOCOL_PIPE,
    /* Client is communicating via serial port. */
    QEMUD_PROTOCOL_SERIAL
} QemudProtocol;

/* Descriptor for a QEMUD pipe connection.
 *
 * Every time a client connects to the QEMUD via pipe, an instance of this
 * structure is created to represent a connection used by new pipe client.
 */
typedef struct QemudPipe {
    /* Pipe descriptor. */
    void* hwpipe;
    /* Looper used for I/O */
    void* looper;
    /* Service for this pipe. */
    QemudService* service;
    /* Client for this pipe. */
    QemudClient* client;
} QemudPipe;

typedef struct QemudSerial QemudSerial;

struct QemudClient {
    /* Defines protocol, used by the client. */
    QemudProtocol protocol;

    /* Fields that are common for all protocols. */
    char* param;
    void* clie_opaque;
    QemudClientRecv clie_recv;
    QemudClientClose clie_close;
    QemudClientSave clie_save;
    QemudClientLoad clie_load;
    QemudService* service;
    QemudClient* next_serv;
    /* next in same service */
    QemudClient* next;
    QemudClient** pref;

    /* framing support */
    int framing;
    ABool need_header;
    ABool closing;
    QemudSink header[1];
    uint8_t header0[FRAME_HEADER_SIZE];
    QemudSink payload[1];

    /* Fields that are protocol-specific. */
    union {
        /* Serial-specific fields. */
        struct {
            int channel;
            QemudSerial* serial;
        } Serial;
        /* Pipe-specific fields. */
        struct {
            QemudPipe* qemud_pipe;
            QemudPipeMessage* messages;
            QemudPipeMessage* last_msg;
        } Pipe;
    } ProtocolSelector;
};

/** HIGH-LEVEL API
 **/

/* remove a QemudClient from global list */
extern void qemud_client_remove(QemudClient* c);

/* add a QemudClient to global list */
extern void qemud_client_prepend(QemudClient* c, QemudClient** plist);

/* receive a new message from a client, and dispatch it to
 * the real service implementation.
 */
extern void qemud_client_recv(void* opaque, uint8_t* msg, int msglen);

/* Sends data to a pipe-based client.
 */
extern void _qemud_pipe_send(QemudClient* client, const uint8_t* msg, int msglen);

/* Frees memory allocated for the qemud client.
 */
extern void _qemud_client_free(QemudClient* c);

/* disconnect a client. this automatically frees the QemudClient.
 * note that this also removes the client from the global list
 * and from its service's list, if any.
 * Param:
 *  opaque - QemuClient instance
 *  guest_close - For pipe clients control whether or not the disconnect is
 *      caused by guest closing the pipe handle (in which case 1 is passed in
 *      this parameter). For serial clients this parameter is ignored.
 */
extern void qemud_client_disconnect(void* opaque, int guest_close);

/* allocate a new QemudClient object
 * NOTE: channel_id valie is used as a selector between serial and pipe clients.
 * Since channel_id < 0 is an invalid value for a serial client, it would
 * indicate that creating client is a pipe client. */
extern QemudClient* qemud_client_alloc(int channel_id,
                   const char* client_param,
                   void* clie_opaque,
                   QemudClientRecv clie_recv,
                   QemudClientClose clie_close,
                   QemudClientSave clie_save,
                   QemudClientLoad clie_load,
                   QemudSerial* serial,
                   QemudClient** pclients);


/* Saves the client state needed to re-establish connections on load.
 * Note that we save only serial clients here. The pipe clients will be
 * saved along with the pipe to which they are attached.
 */
extern void qemud_serial_client_save(Stream* f, QemudClient* c);

/* Loads client state from file, then starts a new client connected to the
 * corresponding service.
 * Note that we load only serial clients here. The pipe clients will be
 * loaded along with the pipe to which they were attached.
 */
extern int qemud_serial_client_load(Stream* f,
                                    QemudService* current_services,
                                    int version);

ANDROID_END_HEADER
