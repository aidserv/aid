// Copyright 2016 The Android Open Source Project
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

#include "android/base/async/DefaultLooper.h"

namespace android {
namespace base {

// A TestLooper is a Looper implementation that allows its user to inspect all
// internals.
//
class TestLooper : public DefaultLooper {
public:
    using DefaultLooper::DefaultLooper;

    // Add some functions to uncover the internal state
    const TimerSet& timers() const;
    const TimerList& activeTimers() const;
    const TimerList& pendingTimers() const;

    const FdWatchSet& fdWatches() const;
    const FdWatchList& pendingFdWatches() const;

    bool exitRequested() const;

    using DefaultLooper::runOneIterationWithDeadlineMs;
};

inline const DefaultLooper::TimerSet& TestLooper::timers() const {
    return mTimers;
}

inline const DefaultLooper::TimerList& TestLooper::activeTimers() const {
    return mActiveTimers;
}

inline const DefaultLooper::TimerList& TestLooper::pendingTimers() const {
    return mPendingTimers;
}

inline const DefaultLooper::FdWatchSet& TestLooper::fdWatches() const {
    return mFdWatches;
}

inline const DefaultLooper::FdWatchList& TestLooper::pendingFdWatches() const {
    return mPendingFdWatches;
}

inline bool TestLooper::exitRequested() const {
    return mForcedExit;
}

}  // namespace base
}  // namespace android
