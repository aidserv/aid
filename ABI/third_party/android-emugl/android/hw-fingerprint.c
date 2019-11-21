/* Copyright (C) 2015 The Android Open Source Project
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

#include "android/hw-fingerprint.h"

#include "android/emulation/android_qemud.h"
#include "android/globals.h"
#include "android/utils/debug.h"
#include "android/utils/misc.h"
#include "android/utils/stream.h"
#include "android/utils/system.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define  E(...)    derror(__VA_ARGS__)
#define  W(...)    dwarning(__VA_ARGS__)
#define  D(...)  VERBOSE_PRINT(init,__VA_ARGS__)
#define  V(...)  VERBOSE_PRINT(init,__VA_ARGS__)

/***********************************************************************************

            All the declarations

***********************************************************************************/

typedef struct HwFingerprintClient   HwFingerprintClient;

typedef struct {
    QemudService*           qemu_listen_service;
    HwFingerprintClient*    fp_clients;
    int32_t     fingerid;
    int32_t     finger_is_on_sensor;
} HwFingerprintService;

struct HwFingerprintClient {
    HwFingerprintClient*    next;
    HwFingerprintService*          fp;
    QemudClient*            qemu_client;
};

static void
_hwFingerprintClient_recv(void* opaque, uint8_t* msg, int msglen,
                      QemudClient*  client );

static void
_hwFingerprintClient_close(void* opaque);

static HwFingerprintClient*
_hwFingerprintClient_new( HwFingerprintService*  fp);

/* the only static variable */
static HwFingerprintService _fingerprintState[1];

static void _hwFingerprint_send();

static QemudClient*
_hwFingerprint_connect(void*  opaque,
                    QemudService*  service,
                    int  channel,
                    const char* client_param);
static void
_hwFingerprintClient_removeFromList(HwFingerprintClient** phead,
        HwFingerprintClient* target);

static void _hwFingerprint_save(Stream* f, QemudService* sv, void* opaque);

static int _hwFingerprint_load(Stream* f, QemudService* sv, void* opaque);
/***********************************************************************************

            All the public methods

***********************************************************************************/

void
android_hw_fingerprint_init( void )
{
    HwFingerprintService*  fp = _fingerprintState;

    if (fp->qemu_listen_service == NULL) {
        fp->qemu_listen_service = qemud_service_register("fingerprintlisten", 0, fp,
                _hwFingerprint_connect, _hwFingerprint_save, _hwFingerprint_load);
        D("%s: fingerprint qemud listen service initialized\n", __FUNCTION__);
    }
}

void
android_hw_fingerprint_touch(int fingerid)
{
    HwFingerprintService*  fp = _fingerprintState;
    D("touched fingerprint sensor with finger id %d\n", fingerid);
    fp->fingerid = fingerid;
    fp->finger_is_on_sensor = 1;
    _hwFingerprint_send();
}

void
android_hw_fingerprint_remove()
{
    HwFingerprintService*  fp = _fingerprintState;
    D("finger removed from the fingerprint sensor\n");
    fp->finger_is_on_sensor = 0;
    _hwFingerprint_send();
}

/***********************************************************************************

            All the static methods

***********************************************************************************/

static void _hwFingerprint_save(Stream* f, QemudService* sv, void* opaque) {
    HwFingerprintService* fp = opaque;
    stream_put_be32(f, fp->fingerid);
    stream_put_be32(f, fp->finger_is_on_sensor);
}

static int _hwFingerprint_load(Stream* f, QemudService* sv, void* opaque) {
    HwFingerprintService* fp = opaque;
    fp->fingerid = stream_get_be32(f);
    fp->finger_is_on_sensor = stream_get_be32(f);
    return 0;
}

static void
_hwFingerprint_send()
{
    HwFingerprintService*  fp = _fingerprintState;
    HwFingerprintClient*    fp_client = fp->fp_clients;
    if (!fp_client)
        return;

    char buffer[128];
    if (fp->finger_is_on_sensor) {
        /* send back the finger id that is on the finger print sensor */
        snprintf(buffer, sizeof(buffer) - 1, "on:%d", fp_client->fp->fingerid);
    } else {
        snprintf(buffer, sizeof(buffer) - 1, "off");
    }
    qemud_client_send(fp_client->qemu_client, (const uint8_t*)buffer,
            strlen(buffer)+1 /*add 1 for '\0'*/);
}

static QemudClient*
_hwFingerprint_connect(void*  opaque,
                    QemudService*  service,
                    int  channel,
                    const char* client_param)
{
    HwFingerprintService*  fp = opaque;
    HwFingerprintClient*    fp_client = _hwFingerprintClient_new(fp);
    QemudClient*    client  = qemud_client_new(service, channel, client_param, fp_client,
                                                _hwFingerprintClient_recv,
                                                _hwFingerprintClient_close,
                                                NULL, /* no save */
                                                NULL /* no load */ );
    qemud_client_set_framing(client, 1);
    fp_client->qemu_client = client;

    D("%s: connect finger print listen is called\n", __FUNCTION__);
    return client;
}

static void
_hwFingerprintClient_recv(void* opaque, uint8_t* msg, int msglen,
                      QemudClient*  client )
{
    /* HwFingerprintClient*       fp_client = opaque; */
    D("got message from guest system fingerprint HAL\n");
    guest_data_partition_mounted = 1;
}

static HwFingerprintClient*
_hwFingerprintClient_new( HwFingerprintService*  fp)
{
    HwFingerprintClient*  fp_client;
    ANEW0(fp_client);
    fp_client->fp = fp;
    fp_client->next = fp->fp_clients;
    fp->fp_clients = fp_client;
    return fp_client;
}

static void
_hwFingerprintClient_close(void* opaque)
{
    HwFingerprintClient*       fp_client = opaque;
    if (fp_client->fp) {
        HwFingerprintClient** pnode = &fp_client->fp->fp_clients;
        _hwFingerprintClient_removeFromList(pnode, fp_client);
        fp_client->next = NULL;
        fp_client->fp = NULL;
    }
    AFREE(fp_client);
}

static void
_hwFingerprintClient_removeFromList(HwFingerprintClient** phead, HwFingerprintClient* target)
{
    for (;;) {
        HwFingerprintClient* node = *phead;
        if (node == NULL)
            break;
        if (node == target) {
            *phead = target->next;
            break;
        }
        phead = &node->next;
    }
}
