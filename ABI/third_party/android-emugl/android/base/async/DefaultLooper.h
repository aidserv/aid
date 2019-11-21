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

#include "android/base/async/Looper.h"
#include "android/base/sockets/SocketWaiter.h"

#include <list>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace android {
namespace base {

// Default looper implementation based on select(). To make sure all timers and
// FD watches execute, run its runWithDeadlineMs() explicitly.
class DefaultLooper : public Looper {
public:
    DefaultLooper();

    ~DefaultLooper() override;

    Duration nowMs(ClockType clockType = ClockType::kHost) override;

    DurationNs nowNs(ClockType clockType = ClockType::kHost) override;

    void forceQuit() override;

    //
    //  F D   W A T C H E S
    //
    class FdWatch : public Looper::FdWatch {
    public:
        FdWatch(DefaultLooper* looper, int fd, Callback callback, void* opaque);

        DefaultLooper* defaultLooper() const;

        ~FdWatch() override;

        void addEvents(unsigned events) override;

        void removeEvents(unsigned events) override;

        unsigned poll() const override;

        // Return true iff this FdWatch is pending execution.
        bool isPending() const;

        // Add this FdWatch to a pending queue.
        void setPending(unsigned events);

        // Remove this FdWatch from a pending queue.
        void clearPending();

        // Fire the watch, i.e. invoke the callback with the right
        // parameters.
        void fire();

    private:
        unsigned mWantedEvents;
        unsigned mLastEvents;
        bool mPending;
    };

    void addFdWatch(FdWatch* watch);

    void delFdWatch(FdWatch* watch);

    void addPendingFdWatch(FdWatch* watch);

    void delPendingFdWatch(FdWatch* watch);

    void updateFdWatch(int fd, unsigned wantedEvents);

    Looper::FdWatch* createFdWatch(int fd,
                                   Looper::FdWatch::Callback callback,
                                   void* opaque) override;

    //
    //  T I M E R S
    //

    class Timer : public Looper::Timer {
    public:
        Timer(DefaultLooper* looper,
              Callback callback,
              void* opaque,
              ClockType clock);

        DefaultLooper* defaultLooper() const;

        ~Timer() override;

        Duration deadline() const;

        void startRelative(Duration deadlineMs) override;

        void startAbsolute(Duration deadlineMs) override;

        void stop() override;

        bool isActive() const override;

        void setPending();

        void clearPending();

        void fire();

        void save(Stream* stream) const;

        void load(Stream* stream);

    private:
        Duration mDeadline;
        bool mPending;
    };

    void addTimer(Timer* timer);

    void delTimer(Timer* timer);

    void enableTimer(Timer* timer);

    void disableTimer(Timer* timer);

    void addPendingTimer(Timer* timer);

    void delPendingTimer(Timer* timer);

    Looper::Timer* createTimer(Looper::Timer::Callback callback,
                               void* opaque,
                               ClockType clock) override;

    //
    // Tasks
    //
    class Task : public Looper::Task {
    public:
        Task(Looper* looper, Looper::Task::Callback&& callback,
             bool selfDeleting = false);

        ~Task();

        void schedule() override;
        void cancel() override;
        void run();

    private:
        const bool mSelfDeleting;
    };

    void addTask(Task* task);
    void delTask(Task* task);

    TaskPtr createTask(TaskCallback&& callback) override;
    void scheduleCallback(TaskCallback&& callback) override;

    //
    //  M A I N   L O O P
    //
    int runWithDeadlineMs(Duration deadlineMs) override;

    typedef std::list<Timer*> TimerList;
    typedef std::unordered_map<Timer*, TimerList::iterator> TimerSet;

    typedef std::list<FdWatch*> FdWatchList;
    typedef std::unordered_map<FdWatch*, FdWatchList::iterator> FdWatchSet;

protected:
    bool runOneIterationWithDeadlineMs(Duration deadlineMs);

    std::unique_ptr<SocketWaiter> mWaiter;
    FdWatchSet mFdWatches;          // Set of all fd watches.
    FdWatchList mPendingFdWatches;  // Queue of pending fd watches.

    TimerSet mTimers;          // Set of all timers.
    TimerList mActiveTimers;   // Sorted list of active timers.
    TimerList mPendingTimers;  // Sorted list of pending timers.

    using TaskSet = std::unordered_set<Task*>;
    TaskSet mScheduledTasks;

    bool mForcedExit = false;
};

}  // namespace base
}  // namespace android
