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

#pragma once

#include <android/base/Compiler.h>
#include <android/base/files/Stream.h>

#include <functional>
#include <memory>
#include <utility>

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>

namespace android {
namespace base {

// A Looper is an abstraction for an event loop that can wait for either
// I/O events on file descriptors, or timers.
//
// One can call Looper::create() to create a new instance, create
// Looper::Timer and Looper::FdWatch instances, then call Looper::run()
// to run the loop until there is nothing more to do.
//
// It's possible to stop a Looper from running by calling
// Looper::forceQuit() from within the event loop.
//
class Looper {
public:
    typedef int64_t Duration;
    typedef uint64_t DurationNs;

    enum {
        kDurationInfinite = INT64_MAX,
    };

    // ClockTypes have to mimic the LooperClockType enum from utils/
    enum class ClockType {
        kRealtime,
        kVirtual,
        kHost
    };

    static const char* clockTypeToString(ClockType clock);

    // Create a new generic Looper instance.
    static Looper* create();

    virtual ~Looper();

    // Return the current time as seen by this looper instance in
    // milliseconds and nanoseconds.
    virtual Duration nowMs(ClockType clockType = ClockType::kHost) = 0;
    virtual DurationNs nowNs(ClockType clockType = ClockType::kHost) = 0;

    // Run the event loop until forceQuit() is called or there is no
    // more registered watchers or timers in the looper.
    void run();

    // A variant of run() that allows to run the event loop only until
    // a fixed deadline has passed. |deadlineMs| is a deadline in
    // milliseconds relative to the current clock used by nowMs().
    // If can be kDurationInfinite to indicate no deadline.
    // Return the reason why the looper stopped:
    //    0           -> normal exit through forceQuit()
    //    EWOULDBLOCK -> no more watchers and timers registered.
    //    ETIMEOUT    -> timeout reached.
    virtual int runWithDeadlineMs(Duration deadlineMs) = 0;

    // A variant of run() that allows to run the event loop only until
    // a certain timeout is milliseconds has passed. Return the reason
    // why the looper stopped:
    //    0           -> normal exit through forceQuit()
    //    EWOULDBLOCK -> no more watchers and timers registered.
    //    ETIMEOUT    -> timeout reached.
    int runWithTimeoutMs(Duration timeoutMs);

    // Call this function from within the event loop to force it to quit
    // as soon as possible. runWithDeadlineMS() and runWithTimeoutMs() will
    // return 0.
    virtual void forceQuit() = 0;

    // Interface class for timers implemented by a Looper instance.
    // Use createTimer() to create these.
    class Timer {
    public:
        // Type of callback function called when the timer expires.
        typedef void (*Callback)(void* opaque, Timer* timer);

        virtual ~Timer();

        // Get the parent looper object
        Looper* parentLooper() const;

        // Start, or restart the timer to expire after |timeout_ms|
        // milliseconds.
        virtual void startRelative(Duration timeout_ms) = 0;

        // Start, or restart the timer to expire at |deadline_ms|
        // milliseconds.
        virtual void startAbsolute(Duration deadline_ms) = 0;

        // Stop the timer.
        virtual void stop() = 0;

        // Returns true iff this timer is active.
        virtual bool isActive() const = 0;

        // Serialization to/from streams
        virtual void save(android::base::Stream* stream) const = 0;
        virtual void load(android::base::Stream* stream) = 0;

    protected:
        Timer(Looper* looper, Callback callback, void* opaque,
              ClockType clock);

        Looper* mLooper;
        Callback mCallback;
        void* mOpaque;
        ClockType mClockType;
    };

    // Create a new timer for this Looper instance.
    virtual Timer* createTimer(Timer::Callback callback, void* opaque,
                               ClockType clock = ClockType::kHost) = 0;

    // Interface class for I/O event watchers on a given file descriptor
    // implemented by this Looper instance.
    class FdWatch {
    public:
        enum {
            kEventRead = (1 << 0),
            kEventWrite = (1 << 1),

            kEventMask = (kEventRead | kEventWrite),
        };

        // Type of function called when an I/O event occurs.
        // |opaque| is the opaque pointer passed at creation.
        // |fd| is the file descriptor.
        // |events| is an event bitmask.
        typedef void (*Callback)(void* opaque, int fd, unsigned events);

        virtual ~FdWatch();

        virtual void addEvents(unsigned events) = 0;

        virtual void removeEvents(unsigned events) = 0;

        inline void wantRead() {
            addEvents(FdWatch::kEventRead);
        }
        inline void wantWrite() {
            addEvents(FdWatch::kEventWrite);
        }
        inline void dontWantRead() {
            removeEvents(FdWatch::kEventRead);
        }
        inline void dontWantWrite() {
            removeEvents(FdWatch::kEventWrite);
        }

        virtual unsigned poll() const = 0;

        int fd() const;

    protected:
        FdWatch(Looper* looper, int fd, Callback callback, void* opaque);

        Looper* mLooper;
        int mFd;
        Callback mCallback;
        void* mOpaque;
    };

    // Create a new FdWatch instance from this looper.
    virtual FdWatch* createFdWatch(int fd,
                                   FdWatch::Callback callback,
                                   void* opaque) = 0;

    // An interface for running a task on the main looper thread with as little
    // delay as possible.
    class Task {
        DISALLOW_COPY_AND_ASSIGN(Task);

    public:
        using Callback = std::function<void()>;

        virtual ~Task();

        // Schedule this task to run on the main looper thread. It will only run
        // once, call schedule() again after that if you need to run it again.
        virtual void schedule() = 0;

        // Cancel the scheduled task. It's OK to call it if the task isn't
        // scheduled.
        virtual void cancel() = 0;

    protected:
        Task(Looper* looper, Callback&& callback);

        Looper* const mLooper;
        const Callback mCallback;
    };

    using TaskPtr = std::unique_ptr<Task>;
    using TaskCallback = Task::Callback;

    // Creates a new Task instance for this looper.
    virtual TaskPtr createTask(TaskCallback&& callback) = 0;

    // Schedules the specified one-shot |callback| to run on the main looper
    // thread and returns (doesn't wait for task to complete).
    virtual void scheduleCallback(TaskCallback&& callback) = 0;

protected:
    // Default constructor is protected. Use create() method to create
    // new generic Looper instances.
    Looper();

private:
    DISALLOW_COPY_AND_ASSIGN(Looper);
};

}  // namespace base
}  // namespace android
