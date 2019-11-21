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

#include "android/base/threads/Thread.h"

#include "android/base/synchronization/Lock.h"
#include "android/base/system/System.h"

#include <gtest/gtest.h>

#include <memory>

namespace android {
namespace base {

namespace {

using ::android::base::Thread;

// A simple thread instance that does nothing at all and exits immediately.
class EmptyThread : public Thread {
public:
    intptr_t main() { return 42; }
};

// A thread that checks if onExit is called
class OnExitThread : public ::android::base::Thread {
public:
    static bool onExitCalled;
    static bool dtorCalled;

    OnExitThread() { onExitCalled = dtorCalled = false; }

    ~OnExitThread() { dtorCalled = true; }

    virtual intptr_t main() { return 42; }

    virtual void onExit() {
        onExitCalled = true;
        delete this;
    }
};

bool OnExitThread::onExitCalled = false;
bool OnExitThread::dtorCalled = false;

class CountingThread : public Thread {
public:
    class State {
    public:
        State() : mLock(), mCount(0) {}
        ~State() {}

        void increment() {
            mLock.lock();
            mCount++;
            mLock.unlock();
        }

        int count() const {
            int ret;
            mLock.lock();
            ret = mCount;
            mLock.unlock();
            return ret;
        }

    private:
        mutable Lock mLock;
        int mCount;
    };

    CountingThread(State* state) : Thread(), mState(state) {}

    intptr_t main() {
        mState->increment();
        return 0;
    }

private:
    State* mState;
};

// A thread that blocks till it's instructed to continue.
class BlockingThread : public Thread {
 public:
    BlockingThread() : Thread(), mLock(), mSharedContinueFlag(new bool) {
        *mSharedContinueFlag = false;
    }

    void unblock() {
        mLock.lock();
        *mSharedContinueFlag = true;
        mLock.unlock();
    }

    intptr_t main() {
        bool continueFlag;
        do {
            mLock.lock();
            continueFlag = *mSharedContinueFlag;
            mLock.unlock();

        } while (!continueFlag);
        return 42;
    }

 private:
    mutable Lock mLock;
    std::unique_ptr<bool> mSharedContinueFlag;
};

}  // namespace

TEST(ThreadTest, SimpleThread) {
    Thread* thread = new EmptyThread();
    intptr_t status;
    // Can't wait on threads before starting them.
    EXPECT_FALSE(thread->wait(&status));
    EXPECT_FALSE(thread->tryWait(&status));

    EXPECT_TRUE(thread);
    EXPECT_TRUE(thread->start());
    status = 0;
    EXPECT_TRUE(thread->wait(&status));
    EXPECT_EQ(42, status);

    // Thread objects are single use.
    EXPECT_FALSE(thread->start());
}

TEST(ThreadTest, MultipleThreads) {
    CountingThread::State state;
    const size_t kMaxThreads = 100;
    Thread* threads[kMaxThreads];

    // Create all threads.
    for (size_t n = 0; n < kMaxThreads; ++n) {
        threads[n] = new CountingThread(&state);
        EXPECT_TRUE(threads[n]) << "thread " << n;
    }

    // Start them all.
    for (size_t n = 0; n < kMaxThreads; ++n) {
        EXPECT_TRUE(threads[n]->start()) << "thread " << n;
    }

    // Wait for them all.
    for (size_t n = 0; n < kMaxThreads; ++n) {
        EXPECT_TRUE(threads[n]->wait(NULL)) << "thread " << n;
    }

    // Check state.
    EXPECT_EQ((int)kMaxThreads, state.count());

    // Delete them all.
    for (size_t n = 0; n < kMaxThreads; ++n) {
        delete threads[n];
    }
}

TEST(ThreadTest, OnExit) {
    OnExitThread* thread = new OnExitThread();
    EXPECT_FALSE(OnExitThread::onExitCalled);
    EXPECT_FALSE(OnExitThread::dtorCalled);
    EXPECT_TRUE(thread->start());
    EXPECT_TRUE(thread->wait(NULL));
    EXPECT_TRUE(OnExitThread::onExitCalled);
    EXPECT_TRUE(OnExitThread::dtorCalled);
}

TEST(ThreadTest, tryWait) {
    BlockingThread thread;
    intptr_t result = 0;
    EXPECT_FALSE(thread.tryWait(&result));
    EXPECT_TRUE(thread.start());
    EXPECT_FALSE(thread.tryWait(&result));
    thread.unblock();

    // This has the potential to hang forever in case of a bug in the
    // Thread under test. So, fail after a reaonsably long attempt.
    static const unsigned kMaxWaitTimeMs = 5000;  // 5 seconds.
    static const unsigned kSleepTimeMs = 10;
    static const unsigned kMaxNumChecks = kMaxWaitTimeMs / kSleepTimeMs;
    unsigned numIters = 0;
    result = 0;
    while(numIters < kMaxNumChecks && !thread.tryWait(&result)) {
        System::get()->sleepMs(kSleepTimeMs);
    }
    EXPECT_LT(numIters, kMaxNumChecks);
    EXPECT_EQ(42, result);
}

class TidSetterThread : public Thread {
public:
    intptr_t main() {
        mTid = getCurrentThreadId();
        return 0;
    }

    unsigned long mTid = 0;
};

TEST(ThreadTest, id) {
    EXPECT_NE(0U, getCurrentThreadId());

    TidSetterThread thread1;
    TidSetterThread thread2;

    EXPECT_TRUE(thread1.start());
    EXPECT_TRUE(thread2.start());
    EXPECT_TRUE(thread1.wait());
    EXPECT_TRUE(thread2.wait());
    EXPECT_NE(0U, thread1.mTid);
    EXPECT_NE(0U, thread2.mTid);
    EXPECT_NE(thread1.mTid, thread2.mTid);
}

}  // namespace base
}  // namespace android
