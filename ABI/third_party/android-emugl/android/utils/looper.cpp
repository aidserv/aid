// Copyright 2014 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/utils/looper.h"

#include "android/base/async/Looper.h"
#include "android/base/async/ThreadLooper.h"
#include "android/base/files/Stream.h"
#include "android/base/sockets/SocketUtils.h"

typedef ::Looper CLooper;
typedef android::base::Looper BaseLooper;

static BaseLooper::ClockType toBaseLooperClockType(LooperClockType clock) {
    static_assert((int) LOOPER_CLOCK_HOST == (int) BaseLooper::ClockType::kHost &&
                  (int) LOOPER_CLOCK_VIRTUAL == (int) BaseLooper::ClockType::kVirtual &&
                  (int) LOOPER_CLOCK_REALTIME == (int) BaseLooper::ClockType::kRealtime,
                  "Values in the Looper::ClockType enumeration are out of sync with "
                          "LooperClockType");

    return static_cast<BaseLooper::ClockType>(clock);
}

static BaseLooper* asBaseLooper(CLooper* looper) {
    return reinterpret_cast<BaseLooper*>(looper);
}

Duration looper_now(Looper* looper) {
    return looper_nowWithClock(looper, LOOPER_CLOCK_HOST);
}

Duration looper_nowWithClock(Looper* looper, LooperClockType clock) {
    return asBaseLooper(looper)->nowMs(toBaseLooperClockType(clock));
}

DurationNs looper_nowNs(Looper* looper) {
    return looper_nowNsWithClock(looper, LOOPER_CLOCK_HOST);
}

DurationNs looper_nowNsWithClock(Looper* looper, LooperClockType clock) {
    return asBaseLooper(looper)->nowNs(toBaseLooperClockType(clock));
}

int looper_runWithDeadline(Looper* looper, Duration deadline) {
    return asBaseLooper(looper)->runWithDeadlineMs(deadline);
}

void looper_forceQuit(Looper* looper) {
    asBaseLooper(looper)->forceQuit();
}

void looper_free(Looper* looper) {
    delete asBaseLooper(looper);
}

/**********************************************************************
 **********************************************************************
 *****
 *****  T I M E R S
 *****
 **********************************************************************
 **********************************************************************/

typedef ::android::base::Looper::Timer BaseTimer;

static BaseTimer* asBaseTimer(LoopTimer* timer) {
    return reinterpret_cast<BaseTimer*>(timer);
}

void loopTimer_stop(LoopTimer* timer) {
    asBaseTimer(timer)->stop();
}

void loopTimer_startAbsolute(LoopTimer* timer, Duration deadline_ms) {
    asBaseTimer(timer)->startAbsolute(deadline_ms);
}

void loopTimer_startRelative(LoopTimer* timer, Duration  timeout_ms) {
    asBaseTimer(timer)->startRelative(timeout_ms);
}

int loopTimer_isActive(LoopTimer* timer) {
    return asBaseTimer(timer)->isActive();
}

void loopTimer_free(LoopTimer* timer) {
    delete asBaseTimer(timer);
}

LoopTimer* loopTimer_new(CLooper* looper,
                         LoopTimerFunc callback,
                         void* opaque) {
    return loopTimer_newWithClock(looper, callback, opaque, LOOPER_CLOCK_HOST);
}

LoopTimer* loopTimer_newWithClock(CLooper* looper,
                                  LoopTimerFunc callback,
                                  void* opaque,
                                  LooperClockType clockType) {
    return reinterpret_cast<LoopTimer*>(
            asBaseLooper(looper)->createTimer(
                    reinterpret_cast<BaseLooper::Timer::Callback>(callback),
                    opaque, toBaseLooperClockType(clockType)));
}

/**********************************************************************
 **********************************************************************
 *****
 *****  I / O
 *****
 **********************************************************************
 **********************************************************************/

typedef android::base::Looper::FdWatch BaseFdWatch;

static BaseFdWatch* asBaseFdWatch(LoopIo* io) {
    return reinterpret_cast<BaseFdWatch*>(io);
}

int loopIo_fd(LoopIo* io) {
    return asBaseFdWatch(io)->fd();
}

void loopIo_wantRead(LoopIo* io) {
    asBaseFdWatch(io)->wantRead();
}

void loopIo_wantWrite(LoopIo* io) {
    asBaseFdWatch(io)->wantWrite();
}

void loopIo_dontWantRead(LoopIo* io) {
    asBaseFdWatch(io)->dontWantRead();
}

void loopIo_dontWantWrite(LoopIo* io) {
    asBaseFdWatch(io)->dontWantWrite();
}

unsigned loopIo_poll(LoopIo* io) {
    return asBaseFdWatch(io)->poll();
};

void loopIo_free(LoopIo* io) {
    delete asBaseFdWatch(io);
}

LoopIo* loopIo_new(CLooper* looper,
                   int fd,
                   LoopIoFunc callback,
                   void* opaque) {
    android::base::socketSetNonBlocking(fd);

    return reinterpret_cast<LoopIo*>(
            asBaseLooper(looper)->createFdWatch(fd, callback, opaque));
}

CLooper* looper_getForThread(void) {
    return reinterpret_cast<CLooper*>(
            android::base::ThreadLooper::get());
}

void looper_setForThread(Looper* looper) {
    android::base::ThreadLooper::setLooper(
            reinterpret_cast<BaseLooper*>(looper));
}

void looper_setForThreadToOwn(Looper* looper) {
    android::base::ThreadLooper::setLooper(
            reinterpret_cast<BaseLooper*>(looper), true);
}

CLooper* looper_newGeneric(void) {
    return reinterpret_cast<CLooper*>(
            ::android::base::Looper::create());
}

void stream_put_timer(::Stream* stream, LoopTimer* timer) {
    asBaseTimer(timer)->save(reinterpret_cast<android::base::Stream*>(stream));
}

void stream_get_timer(::Stream* stream, LoopTimer* timer) {
    asBaseTimer(timer)->load(reinterpret_cast<android::base::Stream*>(stream));
}
