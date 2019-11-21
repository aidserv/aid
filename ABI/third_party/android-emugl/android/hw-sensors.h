/* Copyright (C) 2009 The Android Open Source Project
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

#include <stdint.h>

/* initialize sensor emulation */
extern void  android_hw_sensors_init( void );

/* initialize remote controller for HW sensors. This must be called after
 * init_clocks(), i.e. later than android_hw_sensors_init(). */
extern void android_hw_sensors_init_remote_controller( void );

/* NOTE: Sensor status Error definition, It will be used in the Sensors Command
 *       part in android-qemu1-glue/console.c. Details:
 *       SENSOR_STATUS_NO_SERVICE: "sensors" qemud service is not available/initiated.
 *       SENSOR_STATUS_DISABLED: sensor is disabled.
 *       SENSOR_STATUS_UNKNOWN: wrong sensor name.
 *       SENSOR_STATUS_OK: Everything is OK to the current sensor.
 * */
typedef enum{
    SENSOR_STATUS_NO_SERVICE = -3,
    SENSOR_STATUS_DISABLED   = -2,
    SENSOR_STATUS_UNKNOWN    = -1,
    SENSOR_STATUS_OK         = 0,
} SensorStatus;

/* NOTE: this list must be the same that the one defined in
 *       the sensors_qemu.c source of the libsensors.goldfish.so
 *       library.
 *
 *       DO NOT CHANGE THE ORDER IN THIS LIST, UNLESS YOU INTEND
 *       TO BREAK SNAPSHOTS!
 */
#define  SENSORS_LIST  \
    SENSOR_(ACCELERATION,"acceleration") \
    SENSOR_(GYROSCOPE,"gyroscope") \
    SENSOR_(MAGNETIC_FIELD,"magnetic-field") \
    SENSOR_(ORIENTATION,"orientation") \
    SENSOR_(TEMPERATURE,"temperature") \
    SENSOR_(PROXIMITY,"proximity") \
    SENSOR_(LIGHT,"light") \
    SENSOR_(PRESSURE,"pressure") \
    SENSOR_(HUMIDITY,"humidity") \
    SENSOR_(MAGNETIC_FIELD_UNCALIBRATED,"magnetic-field-uncalibrated") \
    SENSOR_(GYROSCOPE_UNCALIBRATED,"gyroscope-uncalibrated") \

typedef enum {
#define  SENSOR_(x,y)  ANDROID_SENSOR_##x,
    SENSORS_LIST
#undef   SENSOR_
    MAX_SENSORS  /* do not remove */
} AndroidSensor;

extern void  android_hw_sensor_enable( AndroidSensor  sensor );

/* COARSE ORIENTATION VALUES */
typedef enum {
    ANDROID_COARSE_PORTRAIT,            // 0 degrees
    ANDROID_COARSE_REVERSE_LANDSCAPE,   // 90 degrees
    ANDROID_COARSE_REVERSE_PORTRAIT,    // 180 degrees
    ANDROID_COARSE_LANDSCAPE,           // 270 degrees
} AndroidCoarseOrientation;

/* change the coarse orientation value */
extern void android_sensors_set_coarse_orientation( AndroidCoarseOrientation  orient );

/* get sensor values */
extern int android_sensors_get( int sensor_id, float* a, float* b, float* c );

/* set sensor values */
extern int android_sensors_set( int sensor_id, float a, float b, float c );

/* Get sensor id from sensor name */
extern int android_sensors_get_id_from_name( char* sensorname );

/* Get sensor name from sensor id */
extern const char* android_sensors_get_name_from_id( int sensor_id );

/* Get sensor from sensor id */
extern uint8_t android_sensors_get_sensor_status( int sensor_id );
