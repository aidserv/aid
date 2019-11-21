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

#include "android/base/synchronization/Lock.h"

#include "android/base/testing/TestThread.h"

#include <gtest/gtest.h>

namespace android {
namespace base {

// Check that construction and destruction doesn't crash.
TEST(Lock, ConstructionDestruction) {
    Lock lock;
}

// Check that a simple lock + unlock works.
TEST(Lock, LockUnlock) {
    Lock lock;
    lock.lock();
    lock.unlock();
}

// Check that AutoLock compiles and doesn't crash.
TEST(Lock, AutoLock) {
    Lock mutex;
    AutoLock lock(mutex);
}

// Wrapper class for threads. Does not use emugl::Thread intentionally.
// Common data type used by the following tests below.
struct ThreadParams {
    ThreadParams() : mutex(), counter(0) {}

    Lock mutex;
    int counter;
};

// This thread function uses Lock::lock/unlock to synchronize a counter
// increment operation.
static void* threadFunction(void* param) {
    ThreadParams* p = static_cast<ThreadParams*>(param);

    p->mutex.lock();
    p->counter++;
    p->mutex.unlock();
    return NULL;
}

TEST(Lock, Synchronization) {
    const size_t kNumThreads = 2000;
    TestThread* threads[kNumThreads];
    ThreadParams p;

    // Create and launch all threads.
    for (size_t n = 0; n < kNumThreads; ++n) {
        threads[n] = new TestThread(threadFunction, &p);
    }

    // Wait until their completion.
    for (size_t n = 0; n < kNumThreads; ++n) {
        threads[n]->join();
        delete threads[n];
    }

    EXPECT_EQ(static_cast<int>(kNumThreads), p.counter);
}

// This thread function uses a AutoLock to protect the counter.
static void* threadAutoLockFunction(void* param) {
    ThreadParams* p = static_cast<ThreadParams*>(param);

    AutoLock lock(p->mutex);
    p->counter++;
    return NULL;
}

TEST(Lock, AutoLockSynchronization) {
    const size_t kNumThreads = 2000;
    TestThread* threads[kNumThreads];
    ThreadParams p;

    // Create and launch all threads.
    for (size_t n = 0; n < kNumThreads; ++n) {
        threads[n] = new TestThread(threadAutoLockFunction, &p);
    }

    // Wait until their completion.
    for (size_t n = 0; n < kNumThreads; ++n) {
        threads[n]->join();
        delete threads[n];
    }

    EXPECT_EQ(static_cast<int>(kNumThreads), p.counter);
}

}  // namespace base
}  // namespace android
