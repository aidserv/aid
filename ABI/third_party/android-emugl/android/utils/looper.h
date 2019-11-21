/* Copyright (C) 2010 The Android Open Source Project
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
#include "android/utils/stream.h"
#include "android/utils/system.h"

#include <stddef.h>
#include <stdint.h>
#include <limits.h>

ANDROID_BEGIN_HEADER

/* stdint.h will not define necessary macros in some c++ compiler
 * implementations without this macro definition.
 */
#if defined(__cplusplus) && !defined(__STDC_LIMIT_MACROS)
#error "This header requires you to define __STDC_LIMIT_MACROS."
#endif

/**********************************************************************
 **********************************************************************
 *****
 *****  T I M E   R E P R E S E N T A T I O N
 *****
 **********************************************************************
 **********************************************************************/

/* A Duration represents a duration in milliseconds */
typedef int64_t   Duration;

/* High-precision duration, in nanoseconds */
typedef uint64_t  DurationNs;

/* A special Duration value used to mean "infinite" */
#define  DURATION_INFINITE       ((Duration)INT64_MAX)

// Type of the clock used in a Looper object
//  LOOPER_CLOCK_REALTIME - realtime monotonic host clock
//  LOOPER_CLOCK_VIRTUAL - virtual machine guest time, stops when guest is
//      suspended
//  LOOPER_CLOCK_HOST - host time, mimics the current time set in the OS, so
//      is may go back when OS time is set back
//
//  Note: the only time which is always supported is LOOPER_CLOCK_HOST;
//        other types may be not implemented by different times and fall back
//        to LOOPER_CLOCK_HOST

typedef enum {
    LOOPER_CLOCK_REALTIME,
    LOOPER_CLOCK_VIRTUAL,
    LOOPER_CLOCK_HOST
} LooperClockType;

/**********************************************************************
 **********************************************************************
 *****
 *****  E V E N T   L O O P   O B J E C T S
 *****
 **********************************************************************
 **********************************************************************/


/* A Looper is an abstraction for an event loop, which can
 * be implemented in different ways. For example, the UI program may
 * want to implement a custom event loop on top of the SDL event queue,
 * while the QEMU core would implement it on top of QEMU's internal
 * main loop which works differently.
 *
 * Once you have a Looper pointer, you can register "watchers" that
 * will trigger callbacks whenever certain events occur. Supported event
 * types are:
 *
 *   - timer expiration
 *   - i/o file descriptor input/output
 *
 * See the relevant documentation for these below.
 *
 * Once you have registered one or more watchers, you can call looper_run()
 * which will run the event loop until looper_forceQuit() is called from a
 * callback, or no more watchers are registered.
 *
 * You can register/unregister watchers from a callback, or call various
 * Looper methods from them (e.g. looper_now(), looper_forceQuit(), etc..)
 *
 * You can create a new Looper by calling looper_newGeneric(). This provides
 * a default implementation that can be used in all threads.
 *
 * For the QEMU core, you can grab a Looper pointer by calling
 * looper_getForThread() instead. Its implementation relies on top of
 * the QEMU event loop instead.
 */
typedef struct Looper    Looper;

/* Return the Looper instance for the current thread. If none exists,
 * create a new one with looper_newGeneric().
 * IMPORTANT: never call looper_free() on the result of this function.
 */
Looper* looper_getForThread(void);

/* Set the current thread's Looper instance. This is only useful if you
 * have a Looper implementation based on a foreign main loop. */
void looper_setForThread(Looper* looper);

// This version of the set function passes the looper ownership - so it is
// deallocated on thread exit.
void looper_setForThreadToOwn(Looper* looper);

/* Create a new generic looper that can be used in any context / thread. */
Looper*  looper_newGeneric(void);

typedef struct LoopTimer LoopTimer;
typedef void (*LoopTimerFunc)(void* opaque, LoopTimer* timer);

typedef struct LoopIo    LoopIo;
typedef void (*LoopIoFunc)(void* opaque, int fd, unsigned events);

/**********************************************************************
 **********************************************************************
 *****
 *****  T I M E R S
 *****
 **********************************************************************
 **********************************************************************/

/* Initialize a LoopTimer with a callback and an 'opaque' value.
 * Each timer belongs to only one looper object.
 */
LoopTimer* loopTimer_new(Looper*        looper,
                         LoopTimerFunc  callback,
                         void*          opaque);
LoopTimer* loopTimer_newWithClock(Looper* looper,
                                  LoopTimerFunc callback,
                                  void* opaque,
                                  LooperClockType clock);

/* Finalize a LoopTimer */
void loopTimer_free(LoopTimer* timer);

/* Start a timer, i.e. arm it to expire in 'timeout_ms' milliseconds,
 * unless loopTimer_stop() is called before that, or the timer is
 * reprogrammed with another loopTimer_startXXX() call.
 */
void loopTimer_startRelative(LoopTimer* timer, Duration timeout_ms);

/* A variant of loopTimer_startRelative that fires on a given deadline
 * in milliseconds instead. If the deadline already passed, the timer is
 * automatically appended to the list of pending event watchers and will
 * fire as soon as possible. Note that this can cause infinite loops
 * in your code if you're not careful.
 */
void loopTimer_startAbsolute(LoopTimer* timer, Duration deadline_ms);

/* Stop a given timer */
void loopTimer_stop(LoopTimer* timer);

/* Returns true iff the timer is active / started */
int loopTimer_isActive(LoopTimer* timer);

/**********************************************************************
 **********************************************************************
 *****
 *****  F I L E   D E S C R I P T O R S
 *****
 **********************************************************************
 **********************************************************************/

/* Bitmasks about i/o events. Note that errors (e.g. network disconnections)
 * are mapped to both read and write events. The idea is that a read() or
 * write() will return 0 or even -1 on non-blocking file descriptors in this
 * case.
 *
 * You can receive several events at the same time on a single LoopIo
 *
 * Socket connect()s are mapped to LOOP_IO_WRITE events.
 * Socket accept()s are mapped to LOOP_IO_READ events.
 */
enum {
    LOOP_IO_READ  = (1 << 0),
    LOOP_IO_WRITE = (1 << 1),
};

LoopIo* loopIo_new(Looper* looper,
                   int fd,
                   LoopIoFunc callback,
                   void* opaque);

/* Note: This does not close the file descriptor! */
void loopIo_free(LoopIo* io);

int loopIo_fd(LoopIo* io);

/* The following functions are used to indicate whether you want the callback
 * to be fired when there is data to be read or when the file is ready to
 * be written to. */
void loopIo_wantRead(LoopIo* io);
void loopIo_wantWrite(LoopIo* io);
void loopIo_dontWantRead(LoopIo* io);
void loopIo_dontWantWrite(LoopIo* io);

unsigned loopIo_poll(LoopIo* io);

/**********************************************************************
 **********************************************************************
 *****
 *****  L O O P E R
 *****
 **********************************************************************
 **********************************************************************/

/* Return the current looper time in milliseconds. This can be used to
 * compute deadlines for looper_runWithDeadline(). */
Duration looper_now(Looper* looper);
Duration looper_nowWithClock(Looper* looper, LooperClockType clock);

/* Current time in nanoseconds */
DurationNs looper_nowNs(Looper* looper);
DurationNs looper_nowNsWithClock(Looper* looper, LooperClockType clock);

/* Run the event loop, until looper_forceQuit() is called, or there is no
 * more registered watchers for events/timers in the looper, or a certain
 * deadline expires.
 *
 * |deadline_ms| is a deadline in milliseconds.
 *
 * The value returned indicates the reason:
 *    0           -> normal exit through looper_forceQuit()
 *    EWOULDBLOCK -> there are not more watchers registered (the looper
 *                   would loop infinitely)
 *    ETIMEDOUT   -> deadline expired.
 */
int looper_runWithDeadline(Looper* looper, Duration deadline_ms);

/* Run the event loop, until looper_forceQuit() is called, or there is no
 * more registered watchers for events/timers in the looper.
 */
AINLINED void looper_run(Looper* looper) {
    (void) looper_runWithDeadline(looper, DURATION_INFINITE);
}

/* A variant of looper_run() that allows to run the event loop only
 * until a certain timeout in milliseconds has passed.
 *
 * Returns the reason why the looper stopped:
 *    0           -> normal exit through looper_forceQuit()
 *    EWOULDBLOCK -> there are not more watchers registered (the looper
 *                   would loop infinitely)
 *    ETIMEDOUT   -> timeout reached
 *
 */
AINLINED int looper_runWithTimeout(Looper* looper, Duration timeout_ms) {
    if (timeout_ms != DURATION_INFINITE)
        timeout_ms += looper_now(looper);

    return looper_runWithDeadline(looper, timeout_ms);
}

/* Call this function from within the event loop to force it to quit
 * as soon as possible. looper_run() / _runWithTimeout() / _runWithDeadline()
 * will then return 0.
 */
void looper_forceQuit(Looper* looper);

/* Destroy a given looper object. Only works for those created
 * with looper_new(). Cannot be called within looper_run()!!
 *
 * NOTE: This assumes that the user has destroyed all its
 *        timers and ios properly
 */
void looper_free(Looper* looper);

// Serialization/deserialization support
void stream_put_timer(Stream* stream, LoopTimer* timer);
void stream_get_timer(Stream* stream, LoopTimer* timer);

/* */

ANDROID_END_HEADER
