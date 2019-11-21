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

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

#define  VERBOSE_TAG_LIST    \
    _VERBOSE_TAG(init,         "emulator initialization")  \
    _VERBOSE_TAG(console,      "control console")  \
    _VERBOSE_TAG(modem,        "emulated GSM modem")  \
    _VERBOSE_TAG(radio,        "emulated GSM AT Command channel") \
    _VERBOSE_TAG(keys,         "key bindings & presses") \
    _VERBOSE_TAG(slirp,        "internal router/firewall") \
    _VERBOSE_TAG(timezone,     "host timezone detection" ) \
    _VERBOSE_TAG(socket,       "network sockets") \
    _VERBOSE_TAG(proxy,        "network proxy support") \
    _VERBOSE_TAG(audio,        "audio sub-system") \
    _VERBOSE_TAG(audioin,      "audio input backend") \
    _VERBOSE_TAG(audioout,     "audio output backend") \
    _VERBOSE_TAG(surface,      "video surface support") \
    _VERBOSE_TAG(qemud,        "qemud multiplexer daemon") \
    _VERBOSE_TAG(gps,          "emulated GPS") \
    _VERBOSE_TAG(nand_limits,  "nand/flash read/write thresholding") \
    _VERBOSE_TAG(hw_control,   "emulated power/flashlight/led/vibrator") \
    _VERBOSE_TAG(avd_config,   "android virtual device configuration") \
    _VERBOSE_TAG(sensors,      "emulated sensors") \
    _VERBOSE_TAG(memcheck,     "memory checker") \
    _VERBOSE_TAG(camera,       "camera") \
    _VERBOSE_TAG(adevice,      "android device connected via port forwarding") \
    _VERBOSE_TAG(sensors_port, "sensors emulator connected to android device") \
    _VERBOSE_TAG(mtport,       "multi-touch emulator connected to android device") \
    _VERBOSE_TAG(mtscreen,     "multi-touch screen emulation") \
    _VERBOSE_TAG(gles,         "hardware OpenGLES emulation") \
    _VERBOSE_TAG(gles1emu,     "emulated GLESv1 renderer") \
    _VERBOSE_TAG(adbserver,    "ADB server") \
    _VERBOSE_TAG(adbclient,    "ADB QEMU client") \
    _VERBOSE_TAG(adb,          "ADB debugger") \
    _VERBOSE_TAG(asconnector,  "Asynchronous socket connector") \
    _VERBOSE_TAG(asyncsocket,  "Asynchronous socket") \
    _VERBOSE_TAG(sdkctlsocket, "Socket tethering to SdkControl server") \
    _VERBOSE_TAG(updater,      "Update checker")  \
    _VERBOSE_TAG(metrics,      "Metrics reporting") \
    _VERBOSE_TAG(rotation,     "Device rotation debugging") \
    _VERBOSE_TAG(goldfishsync, "Goldfish Sync Device") \
    _VERBOSE_TAG(syncthreads,  "HostGPU Sync Threads") \
    _VERBOSE_TAG(capture,      "Video and audio capture debugging") \
    _VERBOSE_TAG(car,          "Emulated car data") \

#define  _VERBOSE_TAG(x,y)  VERBOSE_##x,
typedef enum {
    VERBOSE_TAG_LIST
    VERBOSE_MAX  /* do not remove */
} VerboseTag;
#undef  _VERBOSE_TAG

extern uint64_t android_verbose;

// Enable/disable verbose logs from the base/* family.
extern void base_enable_verbose_logs();
extern void base_disable_verbose_logs();

#define  VERBOSE_ENABLE(tag)    \
    android_verbose |= (1ULL << VERBOSE_##tag)

#define  VERBOSE_DISABLE(tag)   \
    android_verbose &= (1ULL << VERBOSE_##tag)

#define  VERBOSE_CHECK(tag)    \
    ((android_verbose & (1ULL << VERBOSE_##tag)) != 0)

#define  VERBOSE_CHECK_ANY()    \
    (android_verbose != 0)

#define  VERBOSE_PRINT(tag,...)  \
    do { if (VERBOSE_CHECK(tag)) dprint(__VA_ARGS__); } while (0)

// This omits the "emulator: " prefix.
#define  VERBOSE_DPRINT(tag,format,...)  \
    do { if (VERBOSE_CHECK(tag)) { \
         dprintn(format "\n", \
                 ##__VA_ARGS__); } } while(0)

#define  VERBOSE_FUNCTION_PRINT(tag,format,...)  \
    do { if (VERBOSE_CHECK(tag)) \
         dprintn("emulator: %s: " format "\n", \
                 __func__, ##__VA_ARGS__); } while(0)

// This omits the "emulator: " prefix.
#define  VERBOSE_FUNCTION_DPRINT(tag,format,...)  \
    do { if (VERBOSE_CHECK(tag)) \
         dprintn("%s: " format "\n", \
                 __func__, ##__VA_ARGS__); } while(0)

#define  VERBOSE_TID_PRINT(tag,...)  \
    do { if (VERBOSE_CHECK(tag)) \
         android_tid_function_print(true, NULL, __VA_ARGS__); \
    } while (0)

// This omits the "emulator: " prefix.
#define  VERBOSE_TID_DPRINT(tag,...)  \
    do { if (VERBOSE_CHECK(tag)) \
         android_tid_function_print(false, NULL, __VA_ARGS__); } \
    while (0)

#define  VERBOSE_TID_FUNCTION_PRINT(tag,...) \
    do { if (VERBOSE_CHECK(tag)) \
         android_tid_function_print(true, __func__, __VA_ARGS__); } \
    while (0)

// This omits the "emulator: " prefix.
#define  VERBOSE_TID_FUNCTION_DPRINT(tag,...) \
    do { if (VERBOSE_CHECK(tag)) \
         android_tid_function_print(false, __func__, __VA_ARGS__); } \
    while (0)

/** DEBUG TRACE SUPPORT
 **
 ** Debug messages can be sent by calling these functions:
 **
 ** 'dprint' prints "emulator: ", the message, then appends a '\n'
 ** 'dprintn' prints the message as is
 ** 'dprintnv' is 'dprintn' but allows you to use a va_list argument
 ** 'dwarning' prints "emulator: WARNING: ", then appends a '\n'
 ** 'derror' prints "emulator: ERROR: ", then appends a '\n'
 */

extern void   dprint( const char*  format, ... );
extern void   dprintn( const char*  format, ... );
extern void   dprintnv( const char*  format, va_list  args );
extern void   dwarning( const char*  format, ... );
extern void   derror( const char*  format, ... );

/** MULTITHREADED DEBUG TRACING
 **
 ** 'android_tid_function_print' is for tracing in multi-threaded situations.
 ** It prints "emulator: " or not (depending on |use_emulator_prefix|),
 ** the thread id, a function name (|function|), the message, and finally '\n'.
 */
extern void   android_tid_function_print(bool use_emulator_prefix,
                                         const char* function,
                                         const char*  format, ... );

/** STDOUT/STDERR REDIRECTION
 **
 ** allows you to shut temporarily shutdown stdout/stderr
 ** this is useful to get rid of debug messages from ALSA and esd
 ** on Linux.
 **/

extern void  stdio_disable( void );
extern void  stdio_enable( void );

ANDROID_END_HEADER
