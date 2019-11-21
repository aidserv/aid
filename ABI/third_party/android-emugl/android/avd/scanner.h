/* Copyright 2014 The Android Open Source Project
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

// Implements a simple helper object used to retrieve the list of
// available Android Virtual Devices.
//
// Typical usage is:
//
//     AvdScanner* scanner = avdScanner_new(NULL);
//     while ((name = avdScanner_next(scanner)) != NULL) {
//        printf("%s\n", name);
//     }
//     avdScanner_free(scanner);
//

// Opaque type to an object used to scan all available AVDs
// under $ANDROID_SDK_HOME.
typedef struct AvdScanner AvdScanner;

// Create a new AVD scanner instance, used to parse the directory
// at |sdk_home|. If this parameter is NULL, this will find the
// directory from the environment, e.g. ANDROID_SDK_HOME if it is
// defined, or a platform-specific location (e.g. ~/.android/ on Unix).
AvdScanner* avdScanner_new(const char* sdk_home);

// Return the name of the next AVD detected by the scanner.
// This will be NULL at the end of the scan.
const char* avdScanner_next(AvdScanner* scanner);

// Release an AvdScanner object and associated resources.
// This can be called before the end of a scan.
void avdScanner_free(AvdScanner* scanner);

ANDROID_END_HEADER
