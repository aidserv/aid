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

#include "android/emulation/qemud/android_qemud_client.h"

#include "android/emulation/android_pipe_host.h"
#include "android/emulation/qemud/android_qemud_common.h"
#include "android/emulation/qemud/android_qemud_multiplexer.h"
#include "android/utils/bufprint.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>


QemudClient* qemud_client_new(QemudService* service,
                              int channelId,
                              const char* client_param,
                              void* clie_opaque,
                              QemudClientRecv clie_recv,
                              QemudClientClose clie_close,
                              QemudClientSave clie_save,
                              QemudClientLoad clie_load) {
    QemudMultiplexer* m = qemud_multiplexer;
    QemudClient* c = qemud_client_alloc(channelId,
                                        client_param,
                                        clie_opaque,
                                        clie_recv,
                                        clie_close,
                                        clie_save,
                                        clie_load,
                                        m->serial,
                                        &m->clients);

    qemud_service_add_client(service, c);
    return c;
}

static QemudPipeMessage* _qemud_pipe_alloc_msg(int msglen) {
    auto buf = (QemudPipeMessage*) malloc(msglen + sizeof(QemudPipeMessage));
    if (!buf) {
        return nullptr;
    }

    /* Message starts right after the descriptor. */
    buf->message = (uint8_t*) buf + sizeof(QemudPipeMessage);
    buf->size = msglen;
    buf->offset = 0;
    buf->next = NULL;
    return buf;
}

static void _qemud_pipe_append_msg(QemudClient* client, QemudPipeMessage* buf) {
    if (client->ProtocolSelector.Pipe.last_msg) {
        client->ProtocolSelector.Pipe.last_msg->next = buf;
        client->ProtocolSelector.Pipe.last_msg = buf;
    } else {
        client->ProtocolSelector.Pipe.last_msg =
            client->ProtocolSelector.Pipe.messages = buf;
    }
}

void _qemud_pipe_send(QemudClient* client, const uint8_t* msg, int msglen) {
    int avail, len = msglen;
    int framing = client->framing;

    if (msglen <= 0)
        return;

    D("%s: len=%3d '%s'",
      __FUNCTION__, msglen, quote_bytes((const char*) msg, msglen));

    if (framing) {
        len += FRAME_HEADER_SIZE;
    }

    /* packetize the payload for the serial MTU */
    while (len > 0) {
        avail = len;
        if (avail > MAX_SERIAL_PAYLOAD)
            avail = MAX_SERIAL_PAYLOAD;

        /* insert frame header when needed */
        QemudPipeMessage* frame_msg;
        QemudPipeMessage* pipe_msg;
        if (framing) {
            frame_msg = _qemud_pipe_alloc_msg(FRAME_HEADER_SIZE);
            int2hex(frame_msg->message, FRAME_HEADER_SIZE, msglen);
            _qemud_pipe_append_msg(client, frame_msg);

            avail -= FRAME_HEADER_SIZE;
            len -= FRAME_HEADER_SIZE;
            framing = 0;
        }

        pipe_msg = _qemud_pipe_alloc_msg(avail);

        assert(pipe_msg);
        memcpy(pipe_msg->message, msg, avail);

        TRACE("%s: '%.*s'", __func__, avail, msg);

        _qemud_pipe_append_msg(client, pipe_msg);
        android_pipe_host_signal_wake(
                client->ProtocolSelector.Pipe.qemud_pipe->hwpipe,
                PIPE_WAKE_READ);
        msg += avail;
        len -= avail;
    }
}

void qemud_client_send(QemudClient* client, const uint8_t* msg, int msglen) {
    if (qemud_is_pipe_client(client)) {
        _qemud_pipe_send(client, msg, msglen);
    } else {
        qemud_serial_send(client->ProtocolSelector.Serial.serial,
                          client->ProtocolSelector.Serial.channel,
                          client->framing != 0, msg, msglen);
    }
}

void qemud_client_set_framing(QemudClient* client, int framing) {
    /* release dynamic buffer if we're disabling framing */
    if (client->framing) {
        if (!client->need_header) {
            AFREE(client->payload->buff);
            client->need_header = 1;
        }
    }
    client->framing = !!framing;
}

/* this can be used by a service implementation to close a
 * specific client connection.
 */
void qemud_client_close(QemudClient* client) {
    qemud_client_disconnect(client, 0);
}

bool qemud_is_pipe_client(QemudClient* client) {
    return client->protocol == QEMUD_PROTOCOL_PIPE;
}

/* remove a QemudClient from global list */
void qemud_client_remove(QemudClient* c) {
    c->pref[0] = c->next;
    if (c->next)
        c->next->pref = c->pref;

    c->next = NULL;
    c->pref = &c->next;
}

/* add a QemudClient to global list */
void qemud_client_prepend(QemudClient* c, QemudClient** plist) {
    c->next = *plist;
    c->pref = plist;
    *plist = c;
    if (c->next)
        c->next->pref = &c->next;
}

/* receive a new message from a client, and dispatch it to
 * the real service implementation.
 */
void qemud_client_recv(void* opaque, uint8_t* msg, int msglen) {
    QemudClient* c = static_cast<QemudClient*>(opaque);

    /* no framing, things are simple */
    if (!c->framing) {
        if (c->clie_recv)
            c->clie_recv(c->clie_opaque, msg, msglen, c);
        return;
    }

    /* framing */

#if 1
    /* special case, in 99% of cases, everything is in
     * the incoming message, and we can do all we need
     * directly without dynamic allocation.
     */
    if (msglen > FRAME_HEADER_SIZE &&
        c->need_header == 1 &&
        qemud_sink_needed(c->header) == 0) {
        int len = hex2int(msg, FRAME_HEADER_SIZE);

        if (len >= 0 && msglen == len + FRAME_HEADER_SIZE) {
            if (c->clie_recv)
                c->clie_recv(c->clie_opaque,
                             msg + FRAME_HEADER_SIZE,
                             msglen - FRAME_HEADER_SIZE, c);
            return;
        }
    }
#endif

    while (msglen > 0) {
        uint8_t* data;

        /* read the header */
        if (c->need_header) {
            int frame_size;
            uint8_t* data;

            if (!qemud_sink_fill(c->header, (const uint8_t**) &msg, &msglen))
                break;

            frame_size = hex2int(c->header0, 4);
            if (frame_size == 0) {
                D("%s: ignoring empty frame", __FUNCTION__);
                continue;
            }
            if (frame_size < 0) {
                D("%s: ignoring corrupted frame header '.*s'",
                  __FUNCTION__, FRAME_HEADER_SIZE, c->header0);
                continue;
            }

            /* +1 for terminating zero */
            data = static_cast<uint8_t*>(
                    _android_array_alloc(sizeof(*data), frame_size + 1));
            qemud_sink_reset(c->payload, frame_size, data);
            c->need_header = 0;
            c->header->used = 0;
        }

        /* read the payload */
        if (!qemud_sink_fill(c->payload, (const uint8_t**) &msg, &msglen))
            break;

        c->payload->buff[c->payload->size] = 0;
        c->need_header = 1;
        data = c->payload->buff;

        /* Technically, calling 'clie_recv' can destroy client object 'c'
         * if it decides to close the connection, so ensure we don't
         * use/dereference it after the call. */
        if (c->clie_recv)
            c->clie_recv(c->clie_opaque, c->payload->buff, c->payload->size, c);

        AFREE(data);
    }
}

/* Frees memory allocated for the qemud client.
 */
void _qemud_client_free(QemudClient* c) {
    if (c != NULL) {
        if (qemud_is_pipe_client(c)) {
            /* Free outstanding messages. */
            QemudPipeMessage** msg_list = &c->ProtocolSelector.Pipe.messages;
            while (*msg_list != NULL) {
                QemudPipeMessage* to_free = *msg_list;
                *msg_list = to_free->next;
                free(to_free);
            }
            c->ProtocolSelector.Pipe.last_msg = NULL;
        }
        if (c->param != NULL) {
            free(c->param);
        }
        AFREE(c);
    }
}

/* disconnect a client. this automatically frees the QemudClient.
 * note that this also removes the client from the global list
 * and from its service's list, if any.
 * Param:
 *  opaque - QemuClient instance
 *  guest_close - For pipe clients control whether or not the disconnect is
 *      caused by guest closing the pipe handle (in which case 1 is passed in
 *      this parameter). For serial clients this parameter is ignored.
 */
void qemud_client_disconnect(void* opaque, int guest_close) {
    QemudClient* c = static_cast<QemudClient*>(opaque);

    if (c->closing) {  /* recursive call, exit immediately */
        return;
    }

    if (qemud_is_pipe_client(c) && !guest_close) {
        android_pipe_host_close(c->ProtocolSelector.Pipe.qemud_pipe->hwpipe);
        return;
    }

    c->closing = 1;

    /* remove from current list */
    qemud_client_remove(c);

    if (qemud_is_pipe_client(c)) {
        /* We must NULL the client reference in the QemuPipe for this connection,
         * so if a sudden receive request comes after client has been closed, we
         * don't blow up. */
        c->ProtocolSelector.Pipe.qemud_pipe->client = NULL;
    } else if (c->ProtocolSelector.Serial.channel > 0) {
        /* send a disconnect command to the daemon */
        char tmp[128], * p = tmp, * end = p + sizeof(tmp);
        p = bufprint(tmp, end, "disconnect:%02x",
                     c->ProtocolSelector.Serial.channel);
        qemud_serial_send(c->ProtocolSelector.Serial.serial, 0, 0, (uint8_t*) tmp, p - tmp);
    }

    /* call the client close callback */
    if (c->clie_close) {
        c->clie_close(c->clie_opaque);
        c->clie_close = NULL;
    }
    c->clie_recv = NULL;

    /* remove from service list, if any */
    if (c->service) {
        qemud_service_remove_client(c->service, c);
        c->service = NULL;
    }

    _qemud_client_free(c);
}

/* allocate a new QemudClient object
 * NOTE: channel_id valie is used as a selector between serial and pipe clients.
 * Since channel_id < 0 is an invalid value for a serial client, it would
 * indicate that creating client is a pipe client. */
QemudClient* qemud_client_alloc(int channel_id,
                                const char* client_param,
                                void* clie_opaque,
                                QemudClientRecv clie_recv,
                                QemudClientClose clie_close,
                                QemudClientSave clie_save,
                                QemudClientLoad clie_load,
                                QemudSerial* serial,
                                QemudClient** pclients) {
    QemudClient* c = static_cast<QemudClient*>(android_alloc0(sizeof(*c)));

    if (channel_id < 0) {
        /* Allocating a pipe client. */
        c->protocol = QEMUD_PROTOCOL_PIPE;
        c->ProtocolSelector.Pipe.messages = NULL;
        c->ProtocolSelector.Pipe.last_msg = NULL;
        c->ProtocolSelector.Pipe.qemud_pipe = NULL;
    } else {
        /* Allocating a serial client. */
        c->protocol = QEMUD_PROTOCOL_SERIAL;
        c->ProtocolSelector.Serial.serial = serial;
        c->ProtocolSelector.Serial.channel = channel_id;
    }
    c->param = client_param ? ASTRDUP(client_param) : NULL;
    c->clie_opaque = clie_opaque;
    c->clie_recv = clie_recv;
    c->clie_close = clie_close;
    c->clie_save = clie_save;
    c->clie_load = clie_load;
    c->service = NULL;
    c->next_serv = NULL;
    c->next = NULL;
    c->framing = 0;
    c->need_header = 1;
    qemud_sink_reset(c->header, FRAME_HEADER_SIZE, c->header0);

    qemud_client_prepend(c, pclients);

    return c;
}

/* Saves the client state needed to re-establish connections on load.
 * Note that we save only serial clients here. The pipe clients will be
 * saved along with the pipe to which they are attached.
 */
void qemud_serial_client_save(Stream* f, QemudClient* c) {
    /* save generic information */
    qemud_service_save_name(f, c->service);
    stream_put_string(f, c->param);
    stream_put_be32(f, c->ProtocolSelector.Serial.channel);

    /* save client-specific state */
    if (c->clie_save)
        c->clie_save(f, c, c->clie_opaque);

    /* save framing configuration */
    stream_put_be32(f, c->framing);
    if (c->framing) {
        stream_put_be32(f, c->need_header);
        /* header sink always connected to c->header0, no need to save */
        stream_put_be32(f, FRAME_HEADER_SIZE);
        stream_write(f, c->header0, FRAME_HEADER_SIZE);
        /* payload sink */
        qemud_sink_save(f, c->payload);
        stream_write(f, c->payload->buff, c->payload->size);
    }
}

/* Loads client state from file, then starts a new client connected to the
 * corresponding service.
 * Note that we load only serial clients here. The pipe clients will be
 * loaded along with the pipe to which they were attached.
 */
int qemud_serial_client_load(Stream* f,
                             QemudService* current_services,
                             int version) {
    char* service_name = qemud_service_load_name(f);
    if (service_name == NULL)
        return -EIO;
    char* param = stream_get_string(f);
    /* get current service instance */
    QemudService* sv = qemud_service_find(current_services, service_name);
    if (sv == NULL) {
        D("%s: load failed: unknown service \"%s\"\n",
          __FUNCTION__, service_name);
        return -EIO;
    }

    int channel = stream_get_be32(f);

    if (channel == 0) {
        D("%s: illegal snapshot: client for control channel must no be saved\n",
          __FUNCTION__);
        return -EIO;
    }

    /* re-connect client */
    QemudClient* c = qemud_service_connect_client(sv, channel, param);
    if (c == NULL)
        return -EIO;

    /* load client-specific state */
    int ret;
    if (c->clie_load) if ((ret = c->clie_load(f, c, c->clie_opaque)))
        return ret;  /* load failure */

    /* load framing configuration */
    c->framing = stream_get_be32(f);
    if (c->framing) {

        /* header buffer */
        c->need_header = stream_get_be32(f);
        int header_size = stream_get_be32(f);
        if (header_size > FRAME_HEADER_SIZE) {
            D("%s: load failed: payload buffer requires %d bytes, %d available\n",
              __FUNCTION__, header_size, FRAME_HEADER_SIZE);
            return -EIO;
        }
        int ret;
        if ((ret = stream_read(f, c->header0, header_size)) != header_size) {
            D("%s: frame header buffer load failed: expected %d bytes, got %d\n",
              __FUNCTION__, header_size, ret);
            return -EIO;
        }

        /* payload sink */
        if ((ret = qemud_sink_load(f, c->payload)))
            return ret;

        /* replace payload buffer by saved data */
        if (c->payload->buff) {
            AFREE(c->payload->buff);
        }

        /* +1 for terminating zero */
        c->payload->buff = static_cast<uint8_t*>(
                _android_array_alloc(sizeof(*c->payload->buff), c->payload->size + 1));
        if ((ret = stream_read(f, c->payload->buff, c->payload->size)) !=
            c->payload->size) {
            D("%s: frame payload buffer load failed: expected %d bytes, got %d\n",
              __FUNCTION__, c->payload->size, ret);
            AFREE(c->payload->buff);
            return -EIO;
        }
    }

    return 0;
}
