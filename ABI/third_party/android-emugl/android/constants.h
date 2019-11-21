/* Copyright (C) 2016 The Android Open Source Project
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

/* Maximum numbers of emulators that can run concurrently without
 * forcing their port numbers with the -port option.
 * This is not a hard limit. Instead, when starting up, the program
 * will try to bind to localhost ports 5554 + n*2, for n in
 * [0..MAX_EMULATORS), if it fails, it will refuse to start.
 * You can route around this by using the -port or -ports options
 * to specify the ports manually.
 */
#define MAX_ANDROID_EMULATORS  16

#define ANDROID_CONSOLE_BASEPORT 5554
