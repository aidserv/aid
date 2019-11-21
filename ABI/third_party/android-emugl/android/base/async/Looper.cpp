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

#include "android/base/async/Looper.h"

#include "android/base/async/DefaultLooper.h"

#include <utility>

namespace android {
namespace base {

// static
const char* Looper::clockTypeToString(Looper::ClockType clock) {
    static const char kRealtimeStr[] = "kRealTime";
    static const char kVirtualStr[] = "kVirtual";
    static const char kHostStr[] = "kHost";
    static const char kInvalidStr[] = "Invalid";
    switch (clock) {
        case Looper::ClockType::kRealtime:
            return kRealtimeStr;
        case Looper::ClockType::kVirtual:
            return kVirtualStr;
        case Looper::ClockType::kHost:
            return kHostStr;
        default:
            return kInvalidStr;
    }
}

// static
Looper* Looper::create() {
    return new DefaultLooper();
}

Looper::~Looper() = default;

void Looper::run() {
    runWithDeadlineMs(kDurationInfinite);
}

int Looper::runWithTimeoutMs(Looper::Duration timeoutMs) {
    if (timeoutMs != kDurationInfinite) {
        timeoutMs += nowMs();
    }
    return runWithDeadlineMs(timeoutMs);
}

Looper::Looper() = default;

Looper::Timer::~Timer() = default;

Looper* Looper::Timer::parentLooper() const {
    return mLooper;
}

Looper::Timer::Timer(Looper* looper,
                     Looper::Timer::Callback callback,
                     void* opaque,
                     Looper::ClockType clock)
    : mLooper(looper),
      mCallback(callback),
      mOpaque(opaque),
      mClockType(clock) {}

Looper::FdWatch::~FdWatch() = default;

int Looper::FdWatch::fd() const {
    return mFd;
}

Looper::FdWatch::FdWatch(Looper* looper,
                         int fd,
                         Looper::FdWatch::Callback callback,
                         void* opaque)
    : mLooper(looper), mFd(fd), mCallback(callback), mOpaque(opaque) {}

Looper::Task::Task(Looper* looper, Looper::Task::Callback&& callback)
    : mLooper(looper), mCallback(std::move(callback)) {}

Looper::Task::~Task() = default;

}  // namespace base
}  // namespace android
