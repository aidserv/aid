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

#include "android/base/Compiler.h"
#include "android/base/async/Looper.h"
#include "android/base/threads/Thread.h"
#include "android/base/threads/Types.h"

#include <memory>

namespace android {
namespace base {
namespace internal {

// Base class containing template free implemenatation details of ParallelTask
// to minimize template specialization blowup.
//
// This is an implementation detail. DO NOT use this class directly.
class ParallelTaskBase {
public:
    virtual ~ParallelTaskBase() = default;

protected:
    ParallelTaskBase(android::base::Looper* looper,
                     android::base::Looper::Duration checkTimeoutMs,
                     ThreadFlags flags);

    // API functions.
    bool start();
    bool inFlight() const;

    // |ParallelTask<T>| implements these hooks.
    virtual void taskImpl() = 0;
    virtual void taskDoneImpl() = 0;

private:
    class ManagedThread : public ::android::base::Thread {
    public:
        ManagedThread(ParallelTaskBase* manager, ThreadFlags flags)
            : Thread(flags), mManager(manager) {}

        intptr_t main() {
            mManager->taskImpl();
            // This return value is ignored.
            return 0;
        }

    private:
        ParallelTaskBase* mManager;
    };

    // Called prediodically to |tryJoin| the launched thread.
    static void tryWaitTillJoined(void* opaqueThis,
                                  android::base::Looper::Timer* timer);

    android::base::Looper* mLooper;
    android::base::Looper::Duration mCheckTimeoutMs;
    std::unique_ptr<ManagedThread> mManagedThread;

    bool isRunning = false;
    std::unique_ptr<android::base::Looper::Timer> mTimer;

    DISALLOW_COPY_AND_ASSIGN(ParallelTaskBase);
};

}  // namespace internal
}  // namespace base
}  // namespace android
