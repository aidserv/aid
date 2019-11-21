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
#include "android/car.h"

ANDROID_BEGIN_HEADER

typedef struct QCarDataAgent {
    /* called by host UI for registering a callback when Vehicle Hal on guest
       side sends data. */
    void (*setCarCallback)(car_callback_t, void* context);
    /* called by host UI to send data to guest Vehicle Hal */
    void (*sendCarData)(const char* msg, int len);
} QCarDataAgent;

ANDROID_END_HEADER