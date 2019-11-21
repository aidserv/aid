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

#pragma once

#include "android/utils/debug.h"
#include "android/utils/misc.h"

/* length of the framed header */
#define  FRAME_HEADER_SIZE  4

#define  D(...)    VERBOSE_PRINT(qemud,__VA_ARGS__)
#define  D_ACTIVE  VERBOSE_CHECK(qemud)

/* the TRACE(...) macro is used to dump traffic */
#define  T_ACTIVE   0

#if T_ACTIVE
#define  TRACE(...)    VERBOSE_PRINT(qemud,__VA_ARGS__)
#else
#define  TRACE(...)    ((void)0)
#endif

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

/* define SUPPORT_LEGACY_QEMUD to 1 if you want to support
 * talking to a legacy qemud daemon. See docs/ANDROID-QEMUD.TXT
 * for details.
 */
#ifdef TARGET_ARM
#define  SUPPORT_LEGACY_QEMUD  1
#endif
#ifdef TARGET_I386
#define  SUPPORT_LEGACY_QEMUD  0 /* no legacy support */
#endif

#if SUPPORT_LEGACY_QEMUD
#include "android/telephony/modem.h"
#include "android/telephony/modem_driver.h"
#endif
