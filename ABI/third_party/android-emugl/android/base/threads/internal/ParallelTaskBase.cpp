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

#include "android/base/threads/internal/ParallelTaskBase.h"

namespace android {
namespace base {
namespace internal {

using std::unique_ptr;

ParallelTaskBase::ParallelTaskBase(Looper* looper,
                                   Looper::Duration checkTimeoutMs,
                                   ThreadFlags flags)
    : mLooper(looper),
      mCheckTimeoutMs(checkTimeoutMs),
      mManagedThread(new ManagedThread(this, flags)) {}

bool ParallelTaskBase::start() {
    if (!mManagedThread->start()) {
        return false;
    }
    mTimer.reset(mLooper->createTimer(&tryWaitTillJoined, this));
    mTimer->startRelative(0);
    isRunning = true;
    return true;
}

bool ParallelTaskBase::inFlight() const {
    return isRunning;
}

// static
void ParallelTaskBase::tryWaitTillJoined(void* opaqueThis,
                                         Looper::Timer* timer) {
    auto thisPtr = static_cast<ParallelTaskBase*>(opaqueThis);
    if (!thisPtr->mManagedThread->tryWait(nullptr)) {
        thisPtr->mTimer->startRelative(thisPtr->mCheckTimeoutMs);
        return;
    }

    thisPtr->isRunning = false;
    thisPtr->taskDoneImpl();
}

}  // namespace internal
}  // namespace base
}  // namespace android
