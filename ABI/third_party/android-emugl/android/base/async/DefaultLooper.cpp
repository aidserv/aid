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

#include "android/base/async/DefaultLooper.h"

#include "android/base/Log.h"
#include "android/base/system/System.h"
#include "android/base/sockets/SocketErrors.h"

#include <algorithm>
#include <utility>

namespace android {
namespace base {

DefaultLooper::DefaultLooper() : mWaiter(SocketWaiter::create()) {}

DefaultLooper::~DefaultLooper() {
    // Both FdWatch and Timer delete themselves from pending/active lists and
    // from the global list of objects in their dtors. That's why we can't have
    // a regular foreach loop here.
    while (!mFdWatches.empty()) {
        delete mFdWatches.begin()->first;
    }
    while (!mTimers.empty()) {
        delete mTimers.begin()->first;
    }
}

Looper::Duration DefaultLooper::nowMs(Looper::ClockType clockType) {
    DCHECK(clockType == ClockType::kHost);

    const DurationNs nsTime = nowNs(clockType);
    return nsTime == static_cast<DurationNs>(-1)
                   ? -1
                   : static_cast<Duration>(nsTime / 1000000);
}

Looper::DurationNs DefaultLooper::nowNs(Looper::ClockType clockType) {
    DCHECK(clockType == ClockType::kHost);

    return System::get()->getUnixTimeUs() * 1000ull;
}

void DefaultLooper::forceQuit() {
    mForcedExit = true;
}

void DefaultLooper::addFdWatch(DefaultLooper::FdWatch* watch) {
    mFdWatches.emplace(watch, mPendingFdWatches.end());
}

void DefaultLooper::delFdWatch(DefaultLooper::FdWatch* watch) {
    mFdWatches.erase(watch);
}

void DefaultLooper::addPendingFdWatch(DefaultLooper::FdWatch* watch) {
    mPendingFdWatches.push_back(watch);
    mFdWatches[watch] = std::prev(mPendingFdWatches.end());
}

void DefaultLooper::delPendingFdWatch(DefaultLooper::FdWatch* watch) {
    mPendingFdWatches.erase(mFdWatches[watch]);
}

void DefaultLooper::updateFdWatch(int fd, unsigned wantedEvents) {
    mWaiter->update(fd, wantedEvents);
}

Looper::FdWatch* DefaultLooper::createFdWatch(int fd,
                                          Looper::FdWatch::Callback callback,
                                          void* opaque) {
    return new FdWatch(this, fd, callback, opaque);
}

void DefaultLooper::addTimer(DefaultLooper::Timer* timer) {
    mTimers.emplace(timer, mPendingTimers.end());
}

void DefaultLooper::delTimer(DefaultLooper::Timer* timer) {
    mTimers.erase(timer);
}

void DefaultLooper::enableTimer(DefaultLooper::Timer* timer) {
    // Find the first timer that expires after this one.
    const auto deadline = timer->deadline();
    const auto it = std::find_if(
            mActiveTimers.begin(), mActiveTimers.end(),
            [deadline](const Timer* t) { return t->deadline() > deadline; });
    mTimers[timer] = mActiveTimers.insert(it, timer);
}

void DefaultLooper::disableTimer(DefaultLooper::Timer* timer) {
    // Simply remove from the list.
    mActiveTimers.erase(mTimers[timer]);
}

void DefaultLooper::addPendingTimer(DefaultLooper::Timer* timer) {
    mPendingTimers.push_back(timer);
    mTimers[timer] = std::prev(mPendingTimers.end());
}

void DefaultLooper::delPendingTimer(DefaultLooper::Timer* timer) {
    mPendingTimers.erase(mTimers[timer]);
}

Looper::Timer* DefaultLooper::createTimer(Looper::Timer::Callback callback,
                                          void* opaque,
                                          Looper::ClockType clock) {
    return new DefaultLooper::Timer(this, callback, opaque, clock);
}

void DefaultLooper::addTask(DefaultLooper::Task* task) {
    mScheduledTasks.insert(task);
}

void DefaultLooper::delTask(DefaultLooper::Task* task) {
    mScheduledTasks.erase(task);
}

Looper::TaskPtr DefaultLooper::createTask(Looper::TaskCallback&& callback) {
    return Looper::TaskPtr(new Task(this, std::move(callback)));
}

void DefaultLooper::scheduleCallback(Looper::TaskCallback&& callback) {
    (new Task(this, std::move(callback), true))->schedule();
}

int DefaultLooper::runWithDeadlineMs(Looper::Duration deadlineMs) {
    mForcedExit = false;

    while (!mForcedExit) {
        // Return immediately with EWOULDBLOCK if there are no
        // more timers or watches registered.
        if (mFdWatches.empty() && mActiveTimers.empty() &&
            mScheduledTasks.empty()) {
            return EWOULDBLOCK;
        }

        // Return immediately with ETIMEDOUT if we've overrun the deadline.
        // This can happen in two ways:
        //     - We waited till timeout on some FDs.
        //     - We have no FDs to watch, but some timers overran the looper
        //       deadline.
        if (nowMs() > deadlineMs) {
            return ETIMEDOUT;
        }

        if (!runOneIterationWithDeadlineMs(deadlineMs)) {
            break;
        }
    }

    return 0;
}

bool DefaultLooper::runOneIterationWithDeadlineMs(Looper::Duration deadlineMs) {
    // Compute next deadline from timers.
    Duration nextDeadline = kDurationInfinite;

    auto firstTimerIt = mActiveTimers.begin();
    if (firstTimerIt != mActiveTimers.end()) {
        nextDeadline = (*firstTimerIt)->deadline();
    }

    if (nextDeadline > deadlineMs) {
        nextDeadline = deadlineMs;
    }

    // Wait for fd events until next deadline.
    Duration timeOut = INT64_MAX;
    if (nextDeadline < kDurationInfinite) {
        timeOut = nextDeadline - nowMs();
        if (timeOut < 0)
            timeOut = 0;
    }

    if (!mScheduledTasks.empty()) {
        timeOut = 0;
    }

    // Run all scheduled tasks, and make sure we remove them from the scheduled
    // collection.
    const TaskSet tasks = std::move(mScheduledTasks);
    for (const auto& task : tasks) {
        task->run();
    }

    if (mFdWatches.empty()) {
        // We don't have any FDs to watch, so just sleep till the next
        // timer expires.
        System::get()->sleepMs(timeOut);
    } else {
        int ret = mWaiter->wait(timeOut);
        if (ret < 0) {
            // Error, force stop.
            return false;
        }
        DCHECK(mPendingFdWatches.empty());

        if (ret > 0) {
            // Queue pending FdWatch instances.
            for (;;) {
                unsigned events;
                int fd = mWaiter->nextPendingFd(&events);
                if (fd < 0) {
                    break;
                }

                // Find the FdWatch for this file descriptor.
                // TODO(digit): Improve efficiency with a map?
                const auto fdIt =
                        std::find_if(mFdWatches.begin(), mFdWatches.end(),
                                     [fd](const FdWatchSet::value_type& pair) {
                                         return pair.first->fd() == fd;
                                     });
                if (fdIt != mFdWatches.end()) {
                    fdIt->first->setPending(events);
                }
            }
        }
    }

    // Queue pending expired timers.
    DCHECK(mPendingTimers.empty());

    const Duration kNow = nowMs();
    auto timerIt = mActiveTimers.begin();
    while (timerIt != mActiveTimers.end() && (*timerIt)->deadline() <= kNow) {
        // Remove from active list, add to pending list.
        (*timerIt)->setPending();
        timerIt = mActiveTimers.erase(timerIt);
    }

    // Fire the pending timers, this is done in a separate step
    // because the callback could modify the active timers list.
    while (!mPendingTimers.empty()) {
        Timer* timer = mPendingTimers.front();
        timer->clearPending();
        timer->fire();
    }

    // Fire the pending fd watches. Also done in a separate step
    // since the callbacks could modify
    while (!mPendingFdWatches.empty()) {
        FdWatch* watch = mPendingFdWatches.front();
        watch->clearPending();
        watch->fire();
    }

    return true;
}

DefaultLooper::FdWatch::FdWatch(DefaultLooper* looper,
                            int fd,
                            Looper::FdWatch::Callback callback,
                            void* opaque)
    : Looper::FdWatch(looper, fd, callback, opaque),
      mWantedEvents(0U),
      mLastEvents(0U),
      mPending(false) {
    looper->addFdWatch(this);
}

DefaultLooper* DefaultLooper::FdWatch::defaultLooper() const {
    return reinterpret_cast<DefaultLooper*>(mLooper);
}

DefaultLooper::FdWatch::~FdWatch() {
    dontWantWrite();
    dontWantRead();
    clearPending();
    defaultLooper()->delFdWatch(this);
}

void DefaultLooper::FdWatch::addEvents(unsigned events) {
    events &= kEventMask;
    unsigned newEvents = mWantedEvents | events;
    if (newEvents != mWantedEvents) {
        mWantedEvents = newEvents;
        defaultLooper()->updateFdWatch(mFd, newEvents);
    }
}

void DefaultLooper::FdWatch::removeEvents(unsigned events) {
    events &= kEventMask;
    unsigned newEvents = mWantedEvents & ~events;
    if (newEvents != mWantedEvents) {
        mWantedEvents = newEvents;
        defaultLooper()->updateFdWatch(mFd, newEvents);
        if (!newEvents) {
            clearPending();
        }
    }
    // These events are no longer desired.
    mLastEvents &= ~events;
}

unsigned DefaultLooper::FdWatch::poll() const {
    return mLastEvents;
}

bool DefaultLooper::FdWatch::isPending() const {
    return mPending;
}

void DefaultLooper::FdWatch::setPending(unsigned events) {
    DCHECK(!mPending);
    mPending = true;
    mLastEvents = events & kEventMask;
    defaultLooper()->addPendingFdWatch(this);
}

void DefaultLooper::FdWatch::clearPending() {
    if (mPending) {
        defaultLooper()->delPendingFdWatch(this);
        mPending = false;
    }
}

void DefaultLooper::FdWatch::fire() {
    unsigned events = mLastEvents;
    mLastEvents = 0;
    mCallback(mOpaque, mFd, events);
}

DefaultLooper::Timer::Timer(DefaultLooper* looper,
                        Looper::Timer::Callback callback,
                        void* opaque,
                        Looper::ClockType clock)
    : Looper::Timer(looper, callback, opaque, clock),
      mDeadline(kDurationInfinite),
      mPending(false) {
    // this implementation only supports a host clock
    DCHECK(clock == ClockType::kHost);
    DCHECK(mCallback);
    looper->addTimer(this);
}

DefaultLooper* DefaultLooper::Timer::defaultLooper() const {
    return reinterpret_cast<DefaultLooper*>(mLooper);
}

DefaultLooper::Timer::~Timer() {
    clearPending();
    defaultLooper()->delTimer(this);
}

Looper::Duration DefaultLooper::Timer::deadline() const {
    return mDeadline;
}

void DefaultLooper::Timer::startRelative(Looper::Duration deadlineMs) {
    if (deadlineMs != kDurationInfinite) {
        deadlineMs += mLooper->nowMs(mClockType);
    }
    startAbsolute(deadlineMs);
}

void DefaultLooper::Timer::startAbsolute(Looper::Duration deadlineMs) {
    if (mDeadline != kDurationInfinite) {
        defaultLooper()->disableTimer(this);
    }
    mDeadline = deadlineMs;
    if (mDeadline != kDurationInfinite) {
        defaultLooper()->enableTimer(this);
    }
}

void DefaultLooper::Timer::stop() {
    startAbsolute(kDurationInfinite);
}

bool DefaultLooper::Timer::isActive() const {
    return mDeadline != kDurationInfinite;
}

void DefaultLooper::Timer::setPending() {
    mPending = true;
    defaultLooper()->addPendingTimer(this);
}

void DefaultLooper::Timer::clearPending() {
    if (mPending) {
        defaultLooper()->delPendingTimer(this);
        mPending = false;
        // resets mDeadline so it can be added to active timers list again later
        // without assertion failure inside startAbsolute
        // (defaultLooper->disableTimer)
        mDeadline = kDurationInfinite;
    }
}

void DefaultLooper::Timer::fire() {
    mCallback(mOpaque, this);
}

void DefaultLooper::Timer::save(Stream* stream) const {
    stream->putBe64(static_cast<uint64_t>(mDeadline));
}

void DefaultLooper::Timer::load(Stream* stream) {
    const Duration deadline = static_cast<Duration>(stream->getBe64());
    startAbsolute(deadline);
}

DefaultLooper::Task::Task(Looper* looper,
                          Looper::Task::Callback&& callback,
                          bool selfDeleting)
    : Looper::Task(looper, std::move(callback)), mSelfDeleting(selfDeleting) {}

DefaultLooper::Task::~Task() {
    cancel();
}

void DefaultLooper::Task::schedule() {
    static_cast<DefaultLooper*>(mLooper)->addTask(this);
}

void DefaultLooper::Task::cancel() {
    static_cast<DefaultLooper*>(mLooper)->delTask(this);
}

void DefaultLooper::Task::run() {
    mCallback();
    if (mSelfDeleting) {
        delete this;
    }
}

}  // namespace base
}  // namespace android
