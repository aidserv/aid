/* Copyright (C) 2017 The Android Open Source Project
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
#include "android/car.h"

#include "android/emulation/android_qemud.h"
#include "android/globals.h"
#include "android/utils/debug.h"
#include "android/utils/stream.h"
#include "android/utils/system.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define D(...) VERBOSE_PRINT(car, __VA_ARGS__)

#define DEBUG_TRANSPORT 0

#if DEBUG_TRANSPORT
#define T(...) VERBOSE_PRINT(car, __VA_ARGS__)
#else
#define T(...) ((void)0)
#endif

struct CarDataCallback {
    car_callback_t func;
    void* context;
};

struct HwCar {
    QemudService* service;
    QemudClient* client;  // represents the Vehicle Hal on guest side
    CarDataCallback
            callback;  // the callback to invoke when Vehicle Hal sends data.
};
static HwCar _car[1] = {};

static void _hwCarClient_recv(void* opaque,
                              uint8_t* msg,
                              int msglen,
                              QemudClient* car) {
    T("%s: msg length %d", __func__, msglen);
    HwCar* hwCar = (HwCar*)opaque;

    if (hwCar->callback.func != nullptr) {
        hwCar->callback.func((char*)msg, msglen, hwCar->callback.context);
    }
}

static void _hwCarClient_close(void* opaque) {
    // TODO: handle close
}

static void _hwCarClient_save(Stream* f, QemudClient* client, void* opaque) {
    // TODO
}

static int _hwCarClient_load(Stream* f, QemudClient* client, void* opaque) {
    // TODO
    return 0;
}

static void _hwCar_save(Stream* f, QemudService* sv, void* opaque) {
    // TODO
}

static int _hwCar_load(Stream* f, QemudService* s, void* opaque) {
    // TODO
    return 0;
}

static QemudClient* _hwCar_connect(void* opaque,
                                   QemudService* service,
                                   int channel,
                                   const char* client_param) {
    D("Car client connected");
    HwCar* car = (HwCar*)opaque;
    // Allow only one client -- Vehical Hal
    if (car->client != nullptr) {
        qemud_client_close(car->client);
    }
    QemudClient* client = qemud_client_new(
            service, channel, client_param, opaque, _hwCarClient_recv,
            _hwCarClient_close, _hwCarClient_save, _hwCarClient_load);
    qemud_client_set_framing(client, 1);
    car->client = client;
    return client;
}

void android_car_init(void) {
    HwCar* car = _car;
    if (car->service == nullptr) {
        car->service = qemud_service_register("car", 0, car, _hwCar_connect,
                                              _hwCar_save, _hwCar_load);
        D("%s: car qemud service initialized", __func__);
    } else {
        D("%s: car qemud service already initialized", __func__);
    }
}

void set_car_call_back(car_callback_t func, void* context) {
    HwCar* car = _car;
    if (func == nullptr || context == nullptr) {
        D("%s: callback is invalid", __func__);
        return;
    }
    if (car->callback.func != nullptr) {
        // this shouldn't really happen as this function is only
        // called once during qemu initialization
        D("%s: callback has been set before, overwrite it for now", __func__);
    }
    car->callback.func = func;
    car->callback.context = context;
}

void android_send_car_data(const char* msg, int msgLen) {
    if (_car->service == nullptr) {
        D("pipe service is not initialized");
        return;
    }

    if (_car->client == nullptr) {
        D("no car client is connected");
        return;
    }
    T("%s: Sent message length of %d", __func__, );
    qemud_client_send(_car->client, (const uint8_t*)msg, msgLen);
}
