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

#include "android/base/async/ThreadLooper.h"

#include "android/base/Log.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/threads/ThreadStore.h"

namespace android {
namespace base {

namespace {

class State {
public:
    State(Looper* looper, bool owned) : mLooper(looper), mOwned(owned) {}

    ~State() {
        if (mOwned) {
            delete mLooper;
        }
    }

    Looper* looper() const { return mLooper; }

private:
    Looper* mLooper;
    bool mOwned;
};

class ThreadLooperStore : public ThreadStore<State> {
public:
    bool hasLooper() const {
        return ThreadStoreBase::get() != NULL;
    }

    void setLooper(Looper* looper, bool own) {
        CHECK(!get());
        State* state = new State(looper, own);
        set(state);
    }

    Looper* getLooper() {
        State* state = get();
        if (!state) {
            // Create a generic looper if none exists.
            Looper* looper = Looper::create();
            state = new State(looper, true);
            set(state);
        }
        return state->looper();
    }
};

static LazyInstance<ThreadLooperStore> sStore = LAZY_INSTANCE_INIT;

}  // namespace

// static
Looper* ThreadLooper::get() {
    return sStore->getLooper();
}

// static
void ThreadLooper::setLooper(Looper* looper, bool own) {
    // Sanity checks
    CHECK(looper) << "NULL looper!";

    CHECK(!sStore.hasInstance() || !sStore->hasLooper())
            << "ThreadLooper::get() already called for current thread!";

    sStore->setLooper(looper, own);
}

}  // namespace base
}  // namespace android
