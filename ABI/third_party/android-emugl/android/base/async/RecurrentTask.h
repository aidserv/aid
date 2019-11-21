// Copyright (C) 2015 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "android/base/async/Looper.h"
#include "android/base/Compiler.h"
#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/memory/ScopedPtr.h"

#include <functional>
#include <memory>

namespace android {
namespace base {

// A RecurrentTask is an object that allows you to run a task repeatedly on the
// event loop, until you're done.
// Example:
//
//     class AreWeThereYet {
//     public:
//         AreWeThereYet(Looper* looper) :
//                 mAskRepeatedly(looper,
//                                [this]() { return askAgain(); },
//                                1 * 60 * 1000) {}
//
//         void askAgain() {
//             std::cout << "Are we there yet?" << std::endl;
//         }
//
//         void startHike() {
//             mAskRepeatedly.start();
//         }
//
//     private:
//         RecurrentTask mAskRepeatedly;
//     };
//
// Note: RecurrentTask is meant to execute a task __on the looper thread__.
// It is thread safe though.
class RecurrentTask {
public:
    using TaskFunction = std::function<bool()>;

    RecurrentTask(Looper* looper,
                  TaskFunction function,
                  Looper::Duration taskIntervalMs)
        : mLooper(looper),
          mFunction(function),
          mTaskIntervalMs(taskIntervalMs),
          mTimer(mLooper->createTimer(&RecurrentTask::taskCallback, this)) {}

    ~RecurrentTask() { stopAndWait(); }

    void start() {
        {
            AutoLock lock(mLock);
            mInFlight = true;
        }
        mTimer->startRelative(mTaskIntervalMs);
    }

    void stopAsync() {
        mTimer->stop();

        AutoLock lock(mLock);
        mInFlight = false;
    }

    void stopAndWait() {
        mTimer->stop();

        AutoLock lock(mLock);
        mInFlight = false;

        // Make sure we wait for the pending task to complete if it was running.
        while (mInTimerCallback) {
            mInTimerCondition.wait(&lock);
        }
    }

    bool inFlight() const {
        AutoLock lock(mLock);
        return mInFlight;
    }

protected:
    static void taskCallback(void* opaqueThis, Looper::Timer* timer) {
        const auto self = static_cast<RecurrentTask*>(opaqueThis);
        AutoLock lock(self->mLock);
        self->mInTimerCallback = true;
        const bool inFlight = self->mInFlight;
        lock.unlock();
        const auto undoInTimerCallback =
                makeCustomScopedPtr(self, [&lock](RecurrentTask* self) {
                    if (!lock.isLocked()) {
                        lock.lock();
                    }
                    self->mInTimerCallback = false;
                    self->mInTimerCondition.broadcastAndUnlock(&lock);
                });

        if (!inFlight) {
            return;
        }

        const bool callbackResult = self->mFunction();

        lock.lock();
        if (!callbackResult) {
            self->mInFlight = false;
            return;
        }
        // It is possible that the client code in |mFunction| calls |stop|, so
        // we must double check before reposting the task.
        if (!self->mInFlight) {
            return;
        }
        lock.unlock();
        self->mTimer->startRelative(self->mTaskIntervalMs);
    }

private:
    Looper* const mLooper;
    const TaskFunction mFunction;
    const Looper::Duration mTaskIntervalMs;
    const std::unique_ptr<Looper::Timer> mTimer;

    mutable Lock mLock;
    ConditionVariable mInTimerCondition;
    bool mInTimerCallback = false;
    bool mInFlight = false;
};

}  // namespace base
}  // namespace android
