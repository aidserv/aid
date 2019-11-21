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

#pragma once

#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

typedef struct QAndroidSensorsAgent {
    // Sets the values on a given sensor.
    // Input: |sensorId| determines which sensor's values to set, i.e.
    //                   ANDROID_SENSOR_ACCELERATION, ANDROID_SENSOR_PROXIMITY,
    //                   etc.
    int (*setSensor)(int sensorId, float a, float b, float c);

    // Reads the values from a given sensor.
    // Input: |sensorId| determines which sensor's values to retrieve.
    int (*getSensor)(int sensorId, float *a, float *b, float *c);
} QAndroidSensorsAgent;

ANDROID_END_HEADER
