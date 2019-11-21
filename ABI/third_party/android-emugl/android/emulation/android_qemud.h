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
#include "android/utils/compiler.h"
#include "android/utils/stream.h"

#include <stdbool.h>

ANDROID_BEGIN_HEADER

/* Support for the qemud-based 'services' in the emulator.
 * Please read docs/ANDROID-QEMUD.TXT to understand what this is about.
 */

/* initialize the qemud support code in the emulator
 * |sl| is a serial line qemud should use on its end
 */
extern void android_qemud_init(CSerialLine* sl);

/* A QemudService service is used to connect one or more clients to
 * a given emulator facility. Only one client can be connected at any
 * given time, but the connection can be closed periodically.
 */

typedef struct QemudClient QemudClient;
typedef struct QemudService QemudService;
typedef struct QemudMultiplexer QemudMultiplexer;

extern QemudMultiplexer* const qemud_multiplexer;

/* A function that will be called when the client running in the emulated
 * system has closed its connection to qemud.
 */
typedef void (*QemudClientClose)(void* opaque);

/* A function that will be called when the client sends a message to the
 * service through qemud.
 */
typedef void (*QemudClientRecv)(void* opaque,
                                uint8_t* msg,
                                int msglen,
                                QemudClient* client);

/* A function that will be called when the state of the client should be
 * saved to a snapshot.
 */
typedef void (*QemudClientSave)(Stream* f, QemudClient* client, void* opaque);

/* A function that will be called when the state of the client should be
 * restored from a snapshot.
 */
typedef int (*QemudClientLoad)(Stream* f, QemudClient* client, void* opaque);


/* A function that will be called each time a new client in the emulated
 * system tries to connect to a given qemud service. This should typically
 * call qemud_client_new() to register a new client.
 */
typedef QemudClient* (*QemudServiceConnect)(void* opaque,
                                            QemudService* service,
                                            int channel,
                                            const char* client_param);

/* Register a new client for a given service.
 *
 * This function must be used in the serv_connect callback
 * of a given QemudService object (see qemud_service_register()
 * below). It is used to register a new QemudClient to acknowledge
 * a new client connection.
 *
 * 'clie_opaque', 'clie_recv' and 'clie_close' are used to
 * send incoming client messages to the corresponding service
 * implementation, or notify the service that a client has
 * disconnected.
 * 'clie_opaque' will be sent as the first argument to 'clie_recv' and
 * 'clie_close'
 * 'clie_recv' and 'clie_close' are both optional and may be NULL.
 */
extern QemudClient* qemud_client_new(QemudService* service,
                                     int channel_id,
                                     const char* client_param,
                                     void* clie_opaque,
                                     QemudClientRecv clie_recv,
                                     QemudClientClose clie_close,
                                     QemudClientSave clie_save,
                                     QemudClientLoad clie_load);

/* Send a message to a given qemud client
 */
extern void qemud_client_send(QemudClient* client,
                              const uint8_t* msg,
                              int msglen);

/* enable framing for this client. When TRUE, this will
 * use internally a simple 4-hexchar header before each
 * message exchanged through the serial port.
 */
extern void qemud_client_set_framing(QemudClient* client, int framing);

/* Force-close the connection to a given qemud client.
 */
extern void qemud_client_close(QemudClient* client);

/* check if the client is implemented with a pipe */
extern bool qemud_is_pipe_client(QemudClient* client);

/* A function that will be called when the state of the service should be
 * saved to a snapshot.
 */
typedef void (*QemudServiceSave)(Stream* f,
                                 QemudService* service,
                                 void* opaque);

/* A function that will be called when the state of the service should be
 * restored from a snapshot.
 */
typedef int (*QemudServiceLoad)(Stream* f, QemudService* service, void* opaque);

/* Register a new qemud service.
 * this function is used to register a new named qemud-based
 * service. You must provide 'serv_opaque' and 'serv_connect'
 * which will be called whenever a new client tries to connect
 * to the services.
 *
 * 'serv_opaque' is the first parameter to 'serv_connect'
 *
 * 'serv_connect' shall return NULL if the connection is refused,
 * or a handle to a new QemudClient otherwise. The latter can be
 * created through qemud_client_new() defined above.
 *
 * 'max_clients' is the maximum number of clients accepted by
 * the service concurrently. If this value is 0, then any number
 * of clients can connect.
 */
extern QemudService* qemud_service_register(const char* serviceName,
                                            int max_clients,
                                            void* serv_opaque,
                                            QemudServiceConnect serv_connect,
                                            QemudServiceSave serv_save,
                                            QemudServiceLoad serv_load);

/* Create a new QemudService object */
extern QemudService* qemud_service_new(const char* name,
                                       int max_clients,
                                       void* serv_opaque,
                                       QemudServiceConnect serv_connect,
                                       QemudServiceSave serv_save,
                                       QemudServiceLoad serv_load,
                                       QemudService** pservices);

/* broadcast a given message to all clients of a given QemudService
 */
extern void qemud_service_broadcast(QemudService* sv,
                                    const uint8_t* msg,
                                    int msglen);

/* Load QemuD state from a stream.
 */
extern int qemud_multiplexer_load(QemudMultiplexer* m,
                                  Stream* stream,
                                  int version);

/* Save QemuD state to snapshot.
 *
 * The control channel has no state of its own, other than the local variables
 * in qemud_multiplexer_control_recv. We can therefore safely skip saving it,
 * which spares us dealing with the exception of a client not connected to a
 * service.
 */
extern void qemud_multiplexer_save(QemudMultiplexer* m, Stream* stream);

ANDROID_END_HEADER
