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

#include "android/skin/qt/QtLooper.h"

#include "android/skin/qt/QtLooperImpl.h"

#include <QTime>

#include <errno.h>

namespace android {
namespace qt {
namespace internal {

typedef ::android::base::Looper BaseLooper;
typedef ::android::base::Looper::Timer BaseTimer;
typedef ::android::base::Looper::FdWatch BaseFdWatch;

// A partial implementation of android::base::Looper on top of the Qt main
// event loop. There are few important things here:
//
//  1/ There is a single global Qt event loop per thread. So, all instances
//     returned by createLooper() will really use the same state on a thread!
//
//     In other words, don't call it more than once per thread!
//
//  2/ It is not possible to call the runWithDeadlineMs() method, since
//     the event loop is started in the application's main thread by
//     the executable.
//
//  3/ Clock implementation is limited
//         - Millisecond precision only.
//         - Only ClockType::kHost is supported.
//
//  4/ FdWatch is not implemented yet (as it's not in use yet).

class QtLooper : public BaseLooper {
public:
    QtLooper() : Looper() {}

    //
    // T I M E R S
    //
    // The underlying timer is of Qt::TimerType Qt::CoarseTimer. This gives us
    // almost millisecond accuracy (5% error margin), but is more efficient.
    class Timer : public BaseTimer {
    public:
        virtual void startRelative(Duration timeout_ms) {
            if (timeout_ms == kDurationInfinite) {
                mTimer.stop();
            } else {
                mTimer.start(timeout_ms);
            }
        }

        virtual void startAbsolute(Duration deadline_ms) {
            Q_ASSERT_X(false, "QtLooper::Timer::startAbsolute",
                       "startAbsolute is not supported.");
        }

        virtual void stop() { mTimer.stop(); }

        virtual bool isActive() const { return mTimer.isActive(); }

        virtual void save(android::base::Stream* stream) const {
            qDebug("QtLooper::Timer does not support serialization. Skipped.");
        }

        virtual void load(android::base::Stream* stream) {
            qDebug("QtLooper::Timer does not support deserialization. "
                   "Skipped.");
        }

    protected:
        // |QtLooper| provides method(s) to construct an object of this kind.
        friend class QtLooper;

        Timer(QtLooper* looper,
              BaseTimer::Callback callback,
              void* opaque,
              ClockType clock)
            : BaseTimer(looper, callback, opaque, clock),
              mTimer(callback, opaque, this) {
            mTimer.setSingleShot(true);
        }

    private:
        TimerImpl mTimer;

        DISALLOW_COPY_AND_ASSIGN(Timer);
    };

    virtual BaseTimer* createTimer(BaseTimer::Callback callback,
                                   void* opaque,
                                   ClockType clock) {
        clock = validateClockType(clock);
        return new QtLooper::Timer(this, callback, opaque, clock);
    }

    virtual BaseFdWatch* createFdWatch(int fd,
                                       BaseFdWatch::Callback callback,
                                       void* opaque) {
        Q_ASSERT_X(false, "QtLooper::createFdWatch",
                   "FdWatch is not yet implemented for QtLooper.");
        return nullptr;
    }

    BaseLooper::TaskPtr createTask(
            BaseLooper::TaskCallback&& callback) override {
        return BaseLooper::TaskPtr(new TaskImpl(this, std::move(callback)));
    }

    void scheduleCallback(BaseLooper::TaskCallback&& callback) override {
        (new TaskImpl(this, std::move(callback)))->schedule();
    }

    //
    //  L O O P E R
    //
    virtual Duration nowMs(ClockType clock) {
        clock = validateClockType(clock);
        auto time = QTime::currentTime();
        return time.msec() +
               1000UL * (time.second() +
                         60UL * (time.minute() + 60UL * time.hour()));
    }

    virtual DurationNs nowNs(ClockType clockType) {
        qWarning("QtLooper::nowNs is not supported. Defaulting to nowMs.");
        return nowMs(clockType);
    }

    virtual int runWithDeadlineMs(Duration deadline_ms) {
        qWarning("User cannot call |run*| on a QtLooper event loop");
        errno = ENOSYS;
        return -1;
    }

    virtual void forceQuit() {
        qWarning("User cannot call |forceQuit| on a QtLooper event loop");
    }

protected:
    ClockType validateClockType(ClockType clock) {
        if (clock != ClockType::kHost) {
            qWarning(
                    "QtLooper::Timer does not support %s."
                    "Defaulting to  ClockType::kHost.",
                    Looper::clockTypeToString(clock));
        }
        return ClockType::kHost;
    }

private:
    DISALLOW_COPY_AND_ASSIGN(QtLooper);
};

}  // namespace internal

android::base::Looper* createLooper() {
    return new internal::QtLooper();
}

}  // namespace qt
}  // namespace android
