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

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Set to 'true' to enable logs from the 'modem' component.
extern bool android_telephony_debug_modem;

// Set to 'true' to enable logs from the 'radio' component.
extern bool android_telephony_debug_radio;

// Set to 'true' to enable logs from the 'socket' component.
extern bool android_telephony_debug_socket;

// Send a message to the telephony log. By default, this sends stuff to stderr.
extern void android_telephony_log(const char* fmt, ...);

// Use this macro to send a log message if a given debug component is enabled.
#define ANDROID_TELEPHONY_LOG_ON(component,...) \
    do { \
        if (android_telephony_debug_ ## component) \
            android_telephony_log(__VA_ARGS__); \
    } while (0)

#ifdef __cplusplus
}
#endif
