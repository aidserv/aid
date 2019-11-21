/* Copyright (C) 2008 The Android Open Source Project
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
#include "android/utils/ini.h"
#include "android/hw-lcd.h"

#include <stdint.h>

ANDROID_BEGIN_HEADER

typedef char      hw_bool_t;
typedef int       hw_int_t;
typedef int64_t   hw_disksize_t;
typedef char*     hw_string_t;
typedef double    hw_double_t;

/* these macros are used to define the fields of AndroidHwConfig
 * declared below
 */
#define   HWCFG_BOOL(n,s,d,a,t)       hw_bool_t      n;
#define   HWCFG_INT(n,s,d,a,t)        hw_int_t       n;
#define   HWCFG_STRING(n,s,d,a,t)     hw_string_t    n;
#define   HWCFG_DOUBLE(n,s,d,a,t)     hw_double_t    n;
#define   HWCFG_DISKSIZE(n,s,d,a,t)   hw_disksize_t  n;

typedef struct {
#include "android/avd/hw-config-defs.h"
} AndroidHwConfig;

/* Set all default values, based on the target API level */
void androidHwConfig_init( AndroidHwConfig*  hwConfig,
                           int               apiLevel );

/* reads a hardware configuration file from disk.
 * returns -1 if the file could not be read, or 0 in case of success.
 *
 * note that default values are written to hwConfig if the configuration
 * file doesn't have the corresponding hardware properties.
 */
int androidHwConfig_read(AndroidHwConfig* hwConfig, CIniFile* configFile);

/* Write a hardware configuration to a config file object.
 * Returns 0 in case of success. Note that any value that is set to the
 * default will not bet written.
 */
int androidHwConfig_write(AndroidHwConfig* hwConfig, CIniFile* configFile);

/* Finalize a given hardware configuration */
void androidHwConfig_done( AndroidHwConfig* config );

/* Checks if screen doesn't support touch, or multi-touch */
int  androidHwConfig_isScreenNoTouch( AndroidHwConfig* config );
/* Checks if screen supports touch (but not multi-touch). */
int  androidHwConfig_isScreenTouch( AndroidHwConfig* config );
/* Checks if screen supports multi-touch. */
int  androidHwConfig_isScreenMultiTouch( AndroidHwConfig* config );

/* Returns the Screen Size */
hwLcd_screenSize_t androidHwConfig_getScreenSize(AndroidHwConfig* config);

/* Returns CDD defined minimum heap size in MB */
int androidHwConfig_getMinVmHeapSize(AndroidHwConfig* config, int apiLevel);

// Return an integer indicating if the kernel requires a new device
// naming scheme. More specifically:
//  -1 -> don't know, caller will need to auto-detect.
//   0 -> legacy device naming
//   1 -> new device naming.
//
// The new device naming was allegedly introduced in Linux 3.10 and
// replaces /dev/ttyS<num with /dev/ttyGF<num>. Also see related
// declarations in android/kernel/kernel_utils.h
int androidHwConfig_getKernelDeviceNaming( AndroidHwConfig* config );

// Return an integer indicating is the kernel supports YAFFS2 partition
// images. More specifically:
//  -1 -> don't know, caller will need to auto-detect.
//   0 -> does not support YAFFS2 partitions.
//   1 -> does support YAFFS2 partitions.
int androidHwConfig_getKernelYaffs2Support( AndroidHwConfig* config );

// Return the kernel device prefix for serial ports, depending on
// kernel.newDeviceNaming.
const char* androidHwConfig_getKernelSerialPrefix( AndroidHwConfig* config );

ANDROID_END_HEADER
