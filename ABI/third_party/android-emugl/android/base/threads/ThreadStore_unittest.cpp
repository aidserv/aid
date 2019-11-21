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

#include "android/base/threads/ThreadStore.h"

#include "android/base/synchronization/Lock.h"
#include "android/base/testing/TestThread.h"
#include "android/base/threads/Thread.h"

#include <gtest/gtest.h>

namespace android {
namespace base {

namespace {

// Helper class used to count instance creation and destruction.
class StaticCounter {
public:
    enum {
        kMaxInstances = 1000,
    };

    StaticCounter() {
        AutoLock lock(mLock);
        if (mCreationCount < kMaxInstances)
            mInstances[mCreationCount] = this;
        mCreationCount++;
    }

    ~StaticCounter() {
        AutoLock lock(mLock);
        mDestructionCount++;
    }

    static void reset() {
        AutoLock lock(mLock);
        mCreationCount = 0;
        mDestructionCount = 0;
    }

    static size_t getCreationCount() {
        AutoLock lock(mLock);
        return mCreationCount;
    }

    static size_t getDestructionCount() {
        AutoLock lock(mLock);
        return mDestructionCount;
    }

    static void freeAll() {
        for (size_t n = 0; n < kMaxInstances; ++n)
            delete mInstances[n];
    }

private:
    static Lock mLock;
    static size_t mCreationCount;
    static size_t mDestructionCount;
    static StaticCounter* mInstances[kMaxInstances];
};

typedef ThreadStore<StaticCounter> StaticCounterStore;

Lock StaticCounter::mLock;
size_t StaticCounter::mCreationCount = 0;
size_t StaticCounter::mDestructionCount = 0;
StaticCounter* StaticCounter::mInstances[kMaxInstances];

class MyThread : public Thread {
public:
    MyThread(StaticCounterStore* store) : mStore(store) {}

    virtual intptr_t main() {
        mStore->set(new StaticCounter());
        return 0;
    }
private:
    StaticCounterStore* mStore;
};

}  // namespace

// Just check that we can create a new ThreadStoreBase with an empty
// destructor, and use it in the current thread.
TEST(ThreadStoreBase, MainThreadWithoutDestructor) {
    ThreadStoreBase store(NULL);
    static int x = 42;
    store.set(&x);
    EXPECT_EQ(&x, store.get());
}

// The following test checks that exiting a thread correctly deletes
// any thread-local value stored in it.
static void simplyDestroy(void* value) {
    delete (StaticCounter*) value;
}

static void* simpleThreadFunc(void* param) {
    ThreadStoreBase* store = static_cast<ThreadStoreBase*>(param);
    store->set(new StaticCounter());
    ThreadStoreBase::OnThreadExit();
    return NULL;
}

TEST(ThreadStoreBase, ThreadsWithDestructor) {
    ThreadStoreBase store(simplyDestroy);
    const size_t kNumThreads = 1000;
    TestThread* threads[kNumThreads];
    StaticCounter::reset();

    for (size_t n = 0; n < kNumThreads; ++n) {
        threads[n] = new TestThread(&simpleThreadFunc, &store);
    }
    for (size_t n = 0; n < kNumThreads; ++n) {
        threads[n]->join();
    }

    EXPECT_EQ(kNumThreads, StaticCounter::getCreationCount());
    EXPECT_EQ(kNumThreads, StaticCounter::getDestructionCount());

    for (size_t n = 0; n < kNumThreads; ++n) {
        delete threads[n];
    }
}

TEST(ThreadStoreBase, ThreadsWithoutDestructor) {
    ThreadStoreBase store(NULL);
    const size_t kNumThreads = 1000;
    TestThread* threads[kNumThreads];
    StaticCounter::reset();

    for (size_t n = 0; n < kNumThreads; ++n) {
        threads[n] = new TestThread(&simpleThreadFunc, &store);
    }
    for (size_t n = 0; n < kNumThreads; ++n) {
        threads[n]->join();
    }

    EXPECT_EQ(kNumThreads, StaticCounter::getCreationCount());
    EXPECT_EQ(0U, StaticCounter::getDestructionCount());

    StaticCounter::freeAll();

    for (size_t n = 0; n < kNumThreads; ++n) {
        delete threads[n];
    }
}

TEST(ThreadStore, MainThread) {
    StaticCounter::reset();

    ThreadStore<StaticCounter> store;
    EXPECT_EQ(NULL, store.get());

    StaticCounter* counter = new StaticCounter();
    store.set(counter);
    EXPECT_EQ(counter, store.get());
    EXPECT_EQ(0U, StaticCounter::getDestructionCount());

    store.set(NULL);
    EXPECT_EQ(1U, StaticCounter::getDestructionCount());
}

TEST(ThreadStore, Threads) {
    StaticCounterStore store;
    const size_t kNumThreads = 100;
    MyThread* threads[kNumThreads];
    StaticCounter::reset();

    for (size_t n = 0; n < kNumThreads; ++n) {
        threads[n] = new MyThread(&store);
        threads[n]->start();
    }

    for (size_t n = 0; n < kNumThreads; ++n) {
        EXPECT_TRUE(threads[n]->wait(NULL)) << "Thread " << n;
    }

    EXPECT_EQ(kNumThreads, StaticCounter::getCreationCount());
    EXPECT_EQ(kNumThreads, StaticCounter::getDestructionCount());

    for (size_t n = 0; n < kNumThreads; ++n) {
        delete threads[n];
    }
}

}  // namespace base
}  // namespace android
