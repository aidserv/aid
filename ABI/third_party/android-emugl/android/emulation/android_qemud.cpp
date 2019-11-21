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

#include "android/emulation/android_qemud.h"

#include "android/utils/looper.h"
#include "android/utils/misc.h"
#include "android/utils/panic.h"

#include "android/emulation/android_pipe_host.h"
#include "android/emulation/qemud/android_qemud_client.h"
#include "android/emulation/qemud/android_qemud_common.h"
#include "android/emulation/qemud/android_qemud_multiplexer.h"
#include "android/emulation/qemud/android_qemud_service.h"

#include <algorithm>

#include <assert.h>
#include <stdlib.h>

/* Initializes QEMUD serial interface.
 */
static void _android_qemud_serial_init(CSerialLine* sl) {
    // guard against double initialization
    static CSerialLine* qemud_serial_line = NULL;

    if (qemud_serial_line != NULL) {
        return;
    }

    qemud_serial_line = sl;
    qemud_multiplexer_init(qemud_multiplexer, sl);
}

/*------------------------------------------------------------------------------
 *
 * QEMUD PIPE service callbacks
 *
 * ----------------------------------------------------------------------------*/

/* Saves pending pipe message to the snapshot file. */
static void _save_pipe_message(Stream* f, QemudPipeMessage* msg) {
    stream_put_be32(f, msg->size);
    stream_put_be32(f, msg->offset);
    stream_write(f, msg->message, msg->size);
}

/* Loads pending pipe messages from the snapshot file.
 * Return:
 *  List of pending pipe messages loaded from snapshot, or NULL if snapshot didn't
 *  contain saved messages.
 */
static QemudPipeMessage* _load_pipe_message(Stream* f, QemudPipeMessage** last) {
    QemudPipeMessage* ret = NULL;
    QemudPipeMessage** next = &ret;
    *last = NULL;

    uint32_t size = stream_get_be32(f);
    while (size != 0) {
        QemudPipeMessage* wrk =
                static_cast<QemudPipeMessage*>(android_alloc0(sizeof(*wrk)));
        *last = *next = wrk;
        wrk->size = size;
        wrk->offset = stream_get_be32(f);
        wrk->message = static_cast<uint8_t*>(malloc(wrk->size));
        if (wrk->message == NULL) {
            APANIC("Unable to allocate buffer for pipe's pending message.");
        }
        stream_read(f, wrk->message, wrk->size);
        next = &wrk->next;
        *next = NULL;
        size = stream_get_be32(f);
    }

    return ret;
}

/* This is a callback that gets invoked when guest is connecting to the service.
 *
 * Here we will create a new client as well as pipe descriptor representing new
 * connection.
 */
static void*
_qemudPipe_init(void* hwpipe, void* _looper, const char* args) {
    QemudMultiplexer* m = qemud_multiplexer;
    QemudService* sv = m->services;
    QemudClient* client;
    QemudPipe* pipe = NULL;
    char service_name[512];
    const char* client_args;
    size_t srv_name_len;

    /* 'args' passed in this callback represents name of the service the guest is
     * connecting to. It can't be NULL. */
    if (args == NULL) {
        D("%s: Missing address!", __FUNCTION__);
        return NULL;
    }

    /* 'args' contain service name, and optional parameters for the client that
     * is about to be created in this call. The parameters are separated from the
     * service name wit ':'. Separate service name from the client param. */
    client_args = strchr(args, ':');
    if (client_args != NULL) {
        srv_name_len = std::min(client_args - args,\
                                (intptr_t) sizeof(service_name) - 1);
        client_args++;  // Past the ':'
        if (*client_args == '\0') {
            /* No actual parameters. */
            client_args = NULL;
        }
    } else {
        srv_name_len = std::min(strlen(args), sizeof(service_name) - 1);
    }
    memcpy(service_name, args, srv_name_len);
    service_name[srv_name_len] = '\0';

    /* Lookup registered service by its name. */
    while (sv != NULL && strcmp(sv->name, service_name)) {
        sv = sv->next;
    }
    if (sv == NULL) {
        D("%s: Service '%s' has not been registered!", __FUNCTION__, service_name);
        return NULL;
    }

    /* Create a client for this connection. -1 as a channel ID signals that this
     * is a pipe client. */
    client = qemud_service_connect_client(sv, -1, client_args);
    if (client != NULL) {
        pipe = static_cast<QemudPipe*>(android_alloc0(sizeof(*pipe)));
        pipe->hwpipe = hwpipe;
        pipe->looper = _looper;
        pipe->service = sv;
        pipe->client = client;
        client->ProtocolSelector.Pipe.qemud_pipe = pipe;
    }

    return pipe;
}

/* Called when the guest wants to close the channel.
*/
static void
_qemudPipe_closeFromGuest(void* opaque) {
    QemudPipe* pipe = static_cast<QemudPipe*>(opaque);
    QemudClient* client = pipe->client;
    D("%s", __FUNCTION__);
    if (client != NULL) {
        qemud_client_disconnect(client, 1);
    } else {
        D("%s: Unexpected NULL client", __FUNCTION__);
    }
}

/* Called when the guest has sent some data to the client.
 */
static int
_qemudPipe_sendBuffers(void* opaque,
                       const AndroidPipeBuffer* buffers,
                       int numBuffers) {
    QemudPipe* pipe = static_cast<QemudPipe*>(opaque);
    QemudClient* client = pipe->client;
    size_t transferred = 0;

    if (client == NULL) {
        D("%s: Unexpected NULL client", __FUNCTION__);
        return -1;
    }

    if (numBuffers == 1) {
        /* Simple case: all data are in one buffer. */
        D("%s: %s", __FUNCTION__, quote_bytes((char*) buffers->data, buffers->size));
        qemud_client_recv(client, buffers->data, buffers->size);
        transferred = buffers->size;
    } else {
        /* If there are multiple buffers involved, collect all data in one buffer
         * before calling the high level client. */
        uint8_t* msg, * wrk;
        int n;
        for (n = 0; n < numBuffers; n++) {
            transferred += buffers[n].size;
        }
        msg = static_cast<uint8_t*>(malloc(transferred));
        wrk = msg;
        for (n = 0; n < numBuffers; n++) {
            memcpy(wrk, buffers[n].data, buffers[n].size);
            wrk += buffers[n].size;
        }
        D("%s: %s", __FUNCTION__, quote_bytes((char*) msg, transferred));
        qemud_client_recv(client, msg, transferred);
        free(msg);
    }

    return transferred;
}

/* Called when the guest is reading data from the client.
 */
static int
_qemudPipe_recvBuffers(void* opaque, AndroidPipeBuffer* buffers, int numBuffers) {
    QemudPipe* pipe = static_cast<QemudPipe*>(opaque);
    QemudClient* client = pipe->client;
    QemudPipeMessage** msg_list;
    AndroidPipeBuffer* buff = buffers;
    AndroidPipeBuffer* endbuff = buffers + numBuffers;
    size_t sent_bytes = 0;
    size_t off_in_buff = 0;

    if (client == NULL) {
        D("%s: Unexpected NULL client", __FUNCTION__);
        return -1;
    }

    msg_list = &client->ProtocolSelector.Pipe.messages;
    if (*msg_list == NULL) {
        /* No data to send. Let it block until we wake it up with
         * PIPE_WAKE_READ when service sends data to the client. */
        return PIPE_ERROR_AGAIN;
    }

    /* Fill in goldfish buffers while they are still available, and there are
     * messages in the client's message list. */
    while (buff != endbuff && *msg_list != NULL) {
        QemudPipeMessage* msg = *msg_list;
        /* Message data fiting the current pipe's buffer. */
        size_t to_copy = std::min(msg->size - msg->offset,
                                  buff->size - off_in_buff);
        memcpy(buff->data + off_in_buff, msg->message + msg->offset, to_copy);
        /* Update offsets. */
        off_in_buff += to_copy;
        msg->offset += to_copy;
        sent_bytes += to_copy;
        if (msg->size == msg->offset) {
            /* We're done with the current message. Go to the next one. */
            *msg_list = msg->next;
            free(msg);
        }
        if (off_in_buff == buff->size) {
            /* Current pipe buffer is full. Continue with the next one. */
            buff++;
            off_in_buff = 0;
        }
    }

    if (!*msg_list) {
        client->ProtocolSelector.Pipe.last_msg = NULL;
    }

    D("%s: -> %u (of %u)", __FUNCTION__, sent_bytes, buffers->size);

    return sent_bytes;
}

static unsigned
_qemudPipe_poll(void* opaque) {
    QemudPipe* pipe = static_cast<QemudPipe*>(opaque);
    QemudClient* client = pipe->client;
    unsigned ret = 0;

    if (client != NULL) {
        ret |= PIPE_POLL_OUT;
        if (client->ProtocolSelector.Pipe.messages != NULL) {
            ret |= PIPE_POLL_IN;
        }
    } else {
        D("%s: Unexpected NULL client", __FUNCTION__);
    }

    return ret;
}

static void
_qemudPipe_wakeOn(void* opaque, int flags) {
    QemudPipe* qemud_pipe = (QemudPipe*) opaque;
    QemudClient* c = qemud_pipe->client;
    D("%s: -> %X", __FUNCTION__, flags);
    if (flags & PIPE_WAKE_READ) {
        if (c->ProtocolSelector.Pipe.messages != NULL) {
            android_pipe_host_signal_wake(
                    c->ProtocolSelector.Pipe.qemud_pipe->hwpipe,
                    PIPE_WAKE_READ);
        }
    }
}

static void _qemudPipe_save(void* opaque, Stream* f) {
    QemudPipe* qemud_pipe = (QemudPipe*) opaque;
    QemudClient* c = qemud_pipe->client;
    QemudPipeMessage* msg = c->ProtocolSelector.Pipe.messages;

    /* save generic information */
    qemud_service_save_name(f, c->service);
    stream_put_string(f, c->param);

    /* Save pending messages. */
    while (msg != NULL) {
        _save_pipe_message(f, msg);
        msg = msg->next;
    }
    /* End of pending messages. */
    stream_put_be32(f, 0);

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

static void* _qemudPipe_load(void* hwpipe,
                             void* pipeOpaque,
                             const char* args,
                             Stream* f) {
    QemudPipe* qemud_pipe = NULL;
    char* param;
    char* service_name = qemud_service_load_name(f);
    if (service_name == NULL)
        return NULL;
    /* get service instance for the loading client*/
    QemudService* sv = qemud_service_find(qemud_multiplexer->services,
                                          service_name);
    if (sv == NULL) {
        D("%s: load failed: unknown service \"%s\"\n",
          __FUNCTION__, service_name);
        return NULL;
    }

    /* Load saved parameters. */
    param = stream_get_string(f);

    /* re-connect client */
    QemudClient* c = qemud_service_connect_client(sv, -1, param);
    if (c == NULL)
        return NULL;

    /* Load pending messages. */
    c->ProtocolSelector.Pipe.messages =
            _load_pipe_message(f, &c->ProtocolSelector.Pipe.last_msg);

    /* load client-specific state */
    if (c->clie_load && c->clie_load(f, c, c->clie_opaque)) {
        /* load failure */
        return NULL;
    }

    /* load framing configuration */
    c->framing = stream_get_be32(f);
    if (c->framing) {

        /* header buffer */
        c->need_header = stream_get_be32(f);
        int header_size = stream_get_be32(f);
        if (header_size > FRAME_HEADER_SIZE) {
            D("%s: load failed: payload buffer requires %d bytes, %d available\n",
              __FUNCTION__, header_size, FRAME_HEADER_SIZE);
            return NULL;
        }
        int ret;
        if ((ret = stream_read(f, c->header0, header_size)) != header_size) {
            D("%s: frame header buffer load failed: expected %d bytes, got %d\n",
              __FUNCTION__, header_size, ret);
            return NULL;
        }

        /* payload sink */
        if ((ret = qemud_sink_load(f, c->payload)))
            return NULL;

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
            return NULL;
        }
    }

    /* Associate the client with the pipe. */
    qemud_pipe = static_cast<QemudPipe*>(android_alloc0(sizeof(*qemud_pipe)));
    qemud_pipe->hwpipe = hwpipe;
    qemud_pipe->looper = pipeOpaque;
    qemud_pipe->service = sv;
    qemud_pipe->client = c;
    c->ProtocolSelector.Pipe.qemud_pipe = qemud_pipe;

    return qemud_pipe;
}

/* QEMUD pipe functions.
 */
static const AndroidPipeFuncs _qemudPipe_funcs = {
        _qemudPipe_init,
        _qemudPipe_closeFromGuest,
        _qemudPipe_sendBuffers,
        _qemudPipe_recvBuffers,
        _qemudPipe_poll,
        _qemudPipe_wakeOn,
        _qemudPipe_save,
        _qemudPipe_load,
};

/* Initializes QEMUD pipe interface.
 */
static void _android_qemud_pipe_init(void) {
    static bool _qemud_pipe_initialized = false;

    if (!_qemud_pipe_initialized) {
        android_pipe_add_type("qemud", looper_getForThread(), &_qemudPipe_funcs);
        _qemud_pipe_initialized = true;
    }
}

static bool isInited = false;

void android_qemud_init(CSerialLine* sl) {
    D("%s", __FUNCTION__);
    /* We don't know in advance whether the guest system supports qemud pipes,
     * so we will initialize both qemud machineries, the legacy (over serial
     * port), and the new one (over qemu pipe). Then we let the guest to connect
     * via one, or the other. */
    _android_qemud_serial_init(sl);
    _android_qemud_pipe_init();
    isInited = true;
}

QemudService* qemud_service_register(const char* service_name,
                                     int max_clients,
                                     void* serv_opaque,
                                     QemudServiceConnect serv_connect,
                                     QemudServiceSave serv_save,
                                     QemudServiceLoad serv_load) {
    assert(isInited);

    QemudService* const sv =
            qemud_service_new(service_name,
                              max_clients,
                              serv_opaque,
                              serv_connect,
                              serv_save,
                              serv_load,
                              &qemud_multiplexer->services);
    D("Registered QEMUD service %s", service_name);
    return sv;
}
