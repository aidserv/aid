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

#include "android/emulation/serial_line.h"
#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

/* return the character driver state object that needs to be connected to the
 * emulated serial port where all multiplexed channels go through.
 */
extern CSerialLine* android_qemud_get_serial_line(void);

/* Return in |*psl| a CSerialLine object that will be connected to
 * a single client in the emulated system for a given named service.
 *
 * this is only used to connect GPS and GSM service clients to the
 * implementation that requires a CSerialLine object for legacy
 * reasons.
 *
 * returns 0 on success, or -1 in case of error
 */
extern int android_qemud_get_channel(const char* name, CSerialLine** psl);

/* Set an explicit CSerialLine object for a given qemud communication channel. this
 * is used to attach the channel to an external char driver device (e.g. one
 * created with "-serial <device>") directly.
 *
 * returns 0 on success, -1 on error
 */
extern int android_qemud_set_channel(const char* name, CSerialLine* peer_sl);

/* list of known qemud channel names */
#define  ANDROID_QEMUD_GSM      "gsm"
#define  ANDROID_QEMUD_GPS      "gps"
#define  ANDROID_QEMUD_CONTROL  "control"
#define  ANDROID_QEMUD_SENSORS  "sensors"

ANDROID_END_HEADER
