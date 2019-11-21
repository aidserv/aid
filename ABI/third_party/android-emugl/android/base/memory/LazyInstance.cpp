// Copyright (C) 2014 The Android Open Source Project
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

#include "android/base/memory/LazyInstance.h"
#include "android/base/threads/Thread.h"

namespace android {
namespace base {
namespace internal {

bool LazyInstanceState::inInitState() const {
    return mState.load(std::memory_order_acquire) == State::Init;
}

bool LazyInstanceState::needConstruction() {
    auto state = State::Init;
    if (mState.compare_exchange_strong(state,
                                       State::Constructing,
                                       std::memory_order_acq_rel,
                                       std::memory_order_relaxed)) {
        return true;
    }

    while (state != State::Done) {
        Thread::yield();
        state = mState.load(std::memory_order_acquire);
    }

    return false;
}

void LazyInstanceState::doneConstructing() {
    mState.store(State::Done, std::memory_order_release);
}

}  // namespace internal
}  // namespace base
}  // namespace android
