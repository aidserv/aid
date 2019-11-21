/* Copyright (C) 2007-2008 The Android Open Source Project
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

#include "android/constants.h"
#include "android/avd/info.h"
#include "android/avd/hw-config.h"

/* this structure is setup when loading the virtual device
 * after that, you can read the 'flags' field to determine
 * wether a data or cache wipe has been in effect.
 */
extern AvdInfoParams     android_avdParams[1];

/* a pointer to the android virtual device information
 * object, which can be queried for the paths of various
 * image files or the skin
 */
extern AvdInfo*          android_avdInfo;

/* the hardware configuration for this specific virtual device */
extern AndroidHwConfig   android_hw[1];

/* this is to support snapshot (currently only qemu1+software-renderer) */
extern const char* savevm_on_exit;

/* this indicates that guest has mounted data partition */
extern int guest_data_partition_mounted;
