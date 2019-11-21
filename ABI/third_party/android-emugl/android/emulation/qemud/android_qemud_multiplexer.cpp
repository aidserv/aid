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

#include "android/emulation/qemud/android_qemud_multiplexer.h"

#include "android/emulation/qemud/android_qemud_common.h"
#include "android/utils/bufprint.h"

/* the global multiplexer state */
static QemudMultiplexer mutliplexer_instance;
QemudMultiplexer* const qemud_multiplexer = &mutliplexer_instance;

/* this is the serial_recv callback that is called
 * whenever an incoming message arrives through the serial port
 */
void qemud_multiplexer_serial_recv(void* opaque,
                                   int channel,
                                   uint8_t* msg,
                                   int msglen) {
    QemudMultiplexer* m = (QemudMultiplexer*) opaque;
    // Note: A lock is not needed here.
    QemudClient* c = m->clients;

    /* dispatch to an existing client if possible
     * note that channel 0 is handled by a special
     * QemudClient that is setup in qemud_multiplexer_init()
     */
    for (; c != NULL; c = c->next) {
        if (!qemud_is_pipe_client(c) && c->ProtocolSelector.Serial.channel == channel) {
            qemud_client_recv(c, msg, msglen);
            return;
        }
    }

    D("%s: ignoring %d bytes for unknown channel %d",
      __FUNCTION__, msglen, channel);
}

/* handle a new connection attempt. This returns 0 on
 * success, -1 if the service name is unknown, or -2
 * if the service's maximum number of clients has been
 * reached.
 */
int qemud_multiplexer_connect(QemudMultiplexer* m,
                              const char* service_name,
                              int channel_id) {
    android::base::AutoLock _lock(m->lock);

    /* find the corresponding registered service by name */
    QemudService* sv = qemud_service_find(m->services, service_name);
    if (sv == NULL) {
        D("%s: no registered '%s' service", __FUNCTION__, service_name);
        return -1;
    }

    /* check service's client count */
    if (sv->max_clients > 0 && sv->num_clients >= sv->max_clients) {
        D("%s: registration failed for '%s' service: too many clients (%d)",
          __FUNCTION__, service_name, sv->num_clients);
        return -2;
    }

    /* connect a new client to the service on the given channel */
    if (qemud_service_connect_client(sv, channel_id, NULL) == NULL) {
        return -1;
    }

    return 0;
}

/* disconnect a given client from its channel id */
void qemud_multiplexer_disconnect(QemudMultiplexer* m,
                                  int channel) {
    QemudClient* c;

    /* find the client by its channel id, then disconnect it */
    for (c = m->clients; c; c = c->next) {
        if (!qemud_is_pipe_client(c) && c->ProtocolSelector.Serial.channel == channel) {
            D("%s: disconnecting client %d",
              __FUNCTION__, channel);
            /* note thatt this removes the client from
             * m->clients automatically.
             */
            c->ProtocolSelector.Serial.channel = -1; /* no need to send disconnect:<id> */
            qemud_client_disconnect(c, 0);
            return;
        }
    }
    D("%s: disconnecting unknown channel %d",
      __FUNCTION__, channel);
}

/* disconnects all channels, except for the control channel, without informing
 * the daemon in the guest that disconnection has occurred.
 *
 * Used to silently kill clients when restoring emulator state snapshots.
 */
void qemud_multiplexer_disconnect_noncontrol(QemudMultiplexer* m) {
    QemudClient* c;
    QemudClient* next = m->clients;

    while (next) {
        c = next;
        next = c->next;  /* disconnect frees c, remember next in advance */

        if (!qemud_is_pipe_client(c) && c->ProtocolSelector.Serial.channel > 0) {
            /* skip control channel */
            D("%s: disconnecting client %d",
              __FUNCTION__, c->ProtocolSelector.Serial.channel);
            D("%s: disconnecting client %d\n",
              __FUNCTION__, c->ProtocolSelector.Serial.channel);
            c->ProtocolSelector.Serial.channel = -1; /* do not send disconnect:<id> */
            qemud_client_disconnect(c, 0);
        }
    }
}

/* handle control messages. This is used as the receive
 * callback for the special QemudClient setup to manage
 * channel 0.
 *
 * note that the message is zero-terminated for convenience
 * (i.e. msg[msglen] is a valid memory read that returns '\0')
 */
void qemud_multiplexer_control_recv(void* opaque,
                                    uint8_t* msg,
                                    int msglen,
                                    QemudClient* client) {
    QemudMultiplexer* mult = (QemudMultiplexer*) opaque;
    android::base::AutoLock(mult->lock);
    uint8_t* msgend = msg + msglen;
    char tmp[64], * p = tmp, * end = p + sizeof(tmp);

    /* handle connection attempts.
     * the client message must be "connect:<service-name>:<id>"
     * where <id> is a 2-char hexadecimal string, which must be > 0
     */
    if (msglen > 8 && !memcmp(msg, "connect:", 8)) {
        char* service_name = (char*) msg + 8;
        int channel, ret;
        char* q = strchr(service_name, ':');
        if (q == NULL || q + 3 != (char*) msgend) {
            D("%s: malformed connect message: '%.*s' (offset=%d)",
              __FUNCTION__, msglen, (const char*) msg, q ? q - (char*) msg : -1);
            return;
        }
        *q++ = 0;  /* zero-terminate service name */
        channel = hex2int((uint8_t*) q, 2);
        if (channel <= 0) {
            D("%s: malformed channel id '%.*s",
              __FUNCTION__, 2, q);
            return;
        }

        ret = qemud_multiplexer_connect(mult, service_name, channel);
        /* the answer can be one of:
         *    ok:connect:<id>
         *    ko:connect:<id>:<reason-for-failure>
         */
        if (ret < 0) {
            if (ret == -1) {
                /* could not connect */
                p = bufprint(tmp, end, "ko:connect:%02x:unknown service", channel);
            } else {
                p = bufprint(tmp, end, "ko:connect:%02x:service busy", channel);
            }
        }
        else {
            p = bufprint(tmp, end, "ok:connect:%02x", channel);
        }
        qemud_serial_send(mult->serial, 0, 0, (uint8_t*) tmp, p - tmp);
        return;
    }

    /* handle client disconnections,
     * this message arrives when the client has closed the connection.
     * format: "disconnect:<id>" where <id> is a 2-hex channel id > 0
     */
    if (msglen == 13 && !memcmp(msg, "disconnect:", 11)) {
        int channel_id = hex2int(msg + 11, 2);
        if (channel_id <= 0) {
            D("%s: malformed disconnect channel id: '%.*s'",
              __FUNCTION__, 2, msg + 11);
            return;
        }
        qemud_multiplexer_disconnect(mult, channel_id);
        return;
    }

#if SUPPORT_LEGACY_QEMUD
    /* an ok:connect:<service>:<id> message can be received if we're
     * talking to a legacy qemud daemon, i.e. one running in a 1.0 or
     * 1.1 system image.
     *
     * we should treat is as a normal "connect:" attempt, except that
     * we must not send back any acknowledgment.
     */
    if (msglen > 11 && !memcmp(msg, "ok:connect:", 11)) {
        char*  service_name = (char*)msg + 11;
        char*        q            = strchr(service_name, ':');
        int          channel;

        if (q == NULL || q+3 != (char*)msgend) {
            D("%s: malformed legacy connect message: '%.*s' (offset=%d)",
              __FUNCTION__, msglen, (const char*)msg, q ? q-(char*)msg : -1);
            return;
        }
        *q++ = 0;  /* zero-terminate service name */
        channel = hex2int((uint8_t*)q, 2);
        if (channel <= 0) {
            D("%s: malformed legacy channel id '%.*s",
              __FUNCTION__, 2, q);
            return;
        }

        switch (mult->serial->version) {
        case QEMUD_VERSION_UNKNOWN:
            mult->serial->version = QEMUD_VERSION_LEGACY;
            D("%s: legacy qemud daemon detected.", __FUNCTION__);
            break;

        case QEMUD_VERSION_LEGACY:
            /* nothing unusual */
            break;

        default:
            D("%s: weird, ignoring legacy qemud control message: '%.*s'",
              __FUNCTION__, msglen, msg);
            return;
        }

        /* "hw-control" was called "control" in 1.0/1.1 */
        if (!strcmp(service_name,"control")) {
            // it's safe here: no one will modify the string
            service_name = const_cast<char*>("hw-control");
        }

        qemud_multiplexer_connect(mult, service_name, channel);
        return;
    }

    /* anything else, don't answer for legacy */
    if (mult->serial->version == QEMUD_VERSION_LEGACY) {
        return;
    }
#endif /* SUPPORT_LEGACY_QEMUD */

    /* anything else is a problem */
    p = bufprint(tmp, end, "ko:unknown command");
    qemud_serial_send(mult->serial, 0, 0, (uint8_t*) tmp, p - tmp);
}

/* initialize the global QemudMultiplexer.
 */
void qemud_multiplexer_init(QemudMultiplexer* mult, CSerialLine* serial_line) {
    /* initialize serial handler */
    qemud_serial_init(mult->serial, serial_line,
                      qemud_multiplexer_serial_recv,
                      mult);

    /* setup listener for channel 0 */
    qemud_client_alloc(0,
                       NULL,
                       mult,
                       qemud_multiplexer_control_recv,
                       NULL, NULL, NULL,
                       mult->serial,
                       &mult->clients);
}

/** SNAPSHOT SUPPORT
 **/

/* Saves the number of clients.
 */
static void qemud_client_save_count(Stream* f, QemudClient* c) {
    unsigned int client_count = 0;
    for (; c; c = c->next)   // walk over linked list
        /* skip control channel, which is not saved, and pipe channels that
         * are saved along with the pipe. */
        if (!qemud_is_pipe_client(c) && c->ProtocolSelector.Serial.channel > 0)
            client_count++;

    stream_put_be32(f, client_count);
}

/* Saves the number of services currently available.
 */
static void qemud_service_save_count(Stream* f, QemudService* s) {
    unsigned int service_count = 0;
    for (; s; s = s->next)  // walk over linked list
        service_count++;

    stream_put_be32(f, service_count);
}

/* Removes all active non-control clients, then creates new ones with state
 * taken from the snapshot.
 *
 * We do not send "disconnect" commands, over the channel. If we did, we might
 * stop clients in the restored guest, resulting in an incorrect restore.
 *
 * Instead, we silently replace the clients that were running before the
 * restore with new clients, whose state we copy from the snapshot. Since
 * everything is multiplexed over one link, only the multiplexer notices the
 * changes, there is no communication with the guest.
 */
static int qemud_load_clients(Stream* f, QemudMultiplexer* m, int version) {

    /* Remove all clients, except on the control channel.*/
    qemud_multiplexer_disconnect_noncontrol(m);

    /* Load clients from snapshot */
    int client_count = stream_get_be32(f);
    int i, ret;
    for (i = 0; i < client_count; i++) {
        if ((ret = qemud_serial_client_load(f, m->services, version))) {
            return ret;
        }
    }

    return 0;
}

/* Checks whether the same services are available at this point as when the
 * snapshot was made.
 */
static int qemud_load_services(Stream* f, QemudService* current_services) {
    int i, ret = 0;
    int service_count = stream_get_be32(f);
    for (i = 0; i < service_count; i++) {
        if ((ret = qemud_service_load(f, current_services))) {
            break;
        }
    }
    return ret;
}

int qemud_multiplexer_load(QemudMultiplexer* m,
                           Stream* stream,
                           int version) {
    int ret = 0;
    android::base::AutoLock(m->lock);

    ret = qemud_serial_load(stream, m->serial);
    if (!ret) {
        ret = qemud_load_services(stream, m->services);
        if (!ret) {
            ret = qemud_load_clients(stream, m, version);
        }
    }
    return ret;
}

void qemud_multiplexer_save(QemudMultiplexer* m, Stream* stream) {
    android::base::AutoLock(m->lock);

    /* save serial state if any */
    qemud_serial_save(stream, m->serial);

    /* save service states */
    qemud_service_save_count(stream, m->services);
    QemudService* s;
    for (s = m->services; s; s = s->next)
        qemud_service_save(stream, s);

    /* save client channels */
    qemud_client_save_count(stream, m->clients);
    QemudClient* c;
    for (c = m->clients; c; c = c->next) {
        /* skip control channel, and pipe clients */
        if (!qemud_is_pipe_client(c) && c->ProtocolSelector.Serial.channel > 0) {
            qemud_serial_client_save(stream, c);
        }
    }
}
