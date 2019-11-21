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

#include "android/emulation/qemud/android_qemud_service.h"

#include "android/emulation/qemud/android_qemud_common.h"

#include <errno.h>

QemudService* qemud_service_new(const char* name,
                  int max_clients,
                  void* serv_opaque,
                  QemudServiceConnect serv_connect,
                  QemudServiceSave serv_save,
                  QemudServiceLoad serv_load,
                  QemudService** pservices) {
    QemudService* s = static_cast<QemudService*>(android_alloc0(sizeof(*s)));

    s->name = ASTRDUP(name);
    s->max_clients = max_clients;
    s->num_clients = 0;
    s->clients = NULL;

    s->serv_opaque = serv_opaque;
    s->serv_connect = serv_connect;
    s->serv_save = serv_save;
    s->serv_load = serv_load;

    s->next = *pservices;
    *pservices = s;

    return s;
}

void qemud_service_add_client(QemudService* s, QemudClient* c) {
    c->service = s;
    c->next_serv = s->clients;
    s->clients = c;
    s->num_clients += 1;
}

void qemud_service_remove_client(QemudService* s, QemudClient* c) {
    QemudClient** pnode = &s->clients;
    QemudClient* node;

    /* remove from clients linked-list */
    for (; ;) {
        node = *pnode;
        if (node == NULL) {
            D("%s: could not find client for service '%s'",
              __FUNCTION__, s->name);
            return;
        }
        if (node == c)
            break;
        pnode = &node->next_serv;
    }

    *pnode = node->next_serv;
    s->num_clients -= 1;
}

QemudClient* qemud_service_connect_client(QemudService* sv,
                             int channel_id,
                             const char* client_param) {
    QemudClient* client =
            sv->serv_connect(sv->serv_opaque, sv, channel_id, client_param);
    if (client == NULL) {
        D("%s: registration failed for '%s' service",
          __FUNCTION__, sv->name);
        return NULL;
    }
    D("%s: registered client channel %d for '%s' service",
      __FUNCTION__, channel_id, sv->name);
    return client;
}

QemudService* qemud_service_find(QemudService* service_list, const char* service_name) {
    QemudService* sv = NULL;
    for (sv = service_list; sv != NULL; sv = sv->next) {
        if (!strcmp(sv->name, service_name)) {
            break;
        }
    }
    return sv;
}

void qemud_service_save_name(Stream* f, QemudService* s) {
    int len = strlen(s->name) + 1;  // include '\0' terminator
    stream_put_be32(f, len);
    stream_write(f, (const uint8_t*) s->name, len);
}

char* qemud_service_load_name(Stream* f) {
    int ret;
    int name_len = stream_get_be32(f);
    char* service_name = (char*)android_alloc(name_len);
    if ((ret = stream_read(f, (uint8_t*) service_name, name_len) != name_len)) {
        D("%s: service name load failed: expected %d bytes, got %d\n",
          __FUNCTION__, name_len, ret);
        AFREE(service_name);
        return NULL;
    }
    if (service_name[name_len - 1] != '\0') {
        char last = service_name[name_len - 1];
        service_name[name_len - 1] = '\0';  /* make buffer contents printable */
        D("%s: service name load failed: expecting NULL-terminated string, but "
                  "last char is '%c' (buffer contents: '%s%c')\n",
          __FUNCTION__, name_len, last, service_name, last);
        AFREE(service_name);
        return NULL;
    }

    return service_name;
}

void qemud_service_save(Stream* f, QemudService* s) {
    qemud_service_save_name(f, s);
    stream_put_be32(f, s->max_clients);
    stream_put_be32(f, s->num_clients);

    if (s->serv_save)
        s->serv_save(f, s, s->serv_opaque);
}

int qemud_service_load(Stream* f, QemudService* current_services) {
    char* service_name = qemud_service_load_name(f);
    if (service_name == NULL)
        return -EIO;

    /* get current service instance */
    QemudService* sv = qemud_service_find(current_services, service_name);
    if (sv == NULL) {
        D("%s: loading failed: service \"%s\" not available\n",
          __FUNCTION__, service_name);
        return -EIO;
    }

    /* reconfigure service as required */
    sv->max_clients = stream_get_be32(f);
    sv->num_clients = 0;

    // NOTE: The number of clients saved cannot be verified now.
    (void) stream_get_be32(f);

    /* load service specific data */
    int ret;
    if (sv->serv_load) {
        if ((ret = sv->serv_load(f, sv, sv->serv_opaque))) {
            /* load failure */
            return ret;
        }
    }

    return 0;
}

void qemud_service_broadcast(QemudService* sv,
                                    const uint8_t* msg,
                                    int msglen) {
    QemudClient* c;

    for (c = sv->clients; c; c = c->next_serv)
        qemud_client_send(c, msg, msglen);
}
