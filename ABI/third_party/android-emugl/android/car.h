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
#pragma once
#include "android/utils/compiler.h"

#include <stdlib.h>

ANDROID_BEGIN_HEADER
typedef void (*car_callback_t)(const char*, int, void* context);

/**
 * Set a callback to receive data from guest Vehicle Hal.
 *
 * |func| and |context| cannot be null.
 * TODO: add an unregister callback function.
 */
extern void set_car_call_back(car_callback_t callback, void* context);

/* initialize car data emulation */
extern void android_car_init(void);

/* send data to android vehicle hal */
extern void android_send_car_data(const char* msg, int msgLen);
ANDROID_END_HEADER