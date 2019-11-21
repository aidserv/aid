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

#include "android/utils/compiler.h"
#include <time.h>

ANDROID_BEGIN_HEADER

/* Try to set the default android OS timezone. This operation will affect the
 * emulated networked time in virtual modem
 * returns 0 on success, or -1 if 'tzname' is not in zoneinfo format (e.g. Area/Location)
 */
extern int  timezone_set( const char*  tzname );

/* append the current host "zoneinfo" timezone name to a given buffer. note
 * that this is something like "America/Los_Angeles", and not the human-friendly "PST"
 * this is required by the Android emulated system...
 *
 * the implementation of this function is really tricky and is OS-dependent
 * on Unix systems, it needs to cater to the TZ environment variable, uhhh
 *
 * if TZ is defined to something like "CET" or "PST", this will return the name "Unknown/Unknown"
 */
extern char*  bufprint_zoneinfo_timezone( char*  buffer, char*  end );

/* Return the timezone offset including day light saving in seconds with respect to
 * UTC in guest android OS
 * + positive if the timezone is east of GMT
 * - negative if the timezone is west of GMT
 */
extern long android_tzoffset_in_seconds(time_t* time);

/* Return local time based on android timezone NOT host OS.
 * the caller must dereference the returned parameter struct tm* before use.
 */
extern struct tm* android_localtime(time_t* time);

ANDROID_END_HEADER
