// Copyright (C) 2016 The Android Open Source Project
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

#include "android/base/synchronization/ConditionVariable.h"

#include "android/base/synchronization/Lock.h"
#include "android/base/testing/TestThread.h"

#include <gtest/gtest.h>

#include <vector>

namespace android {
namespace base {

TEST(ConditionVariable, init) {
    ConditionVariable cond;
}

TEST(ConditionVariable, initAndSignal) {
    ConditionVariable cond;
    cond.signal();
}

struct TestThreadParams {
    TestThreadParams() : mutex(), cv(), counter(0) { }
    Lock mutex;
    ConditionVariable cv;
    int counter;
};

static void* testThreadFunctionBasicWait(void* param) {
    TestThreadParams* p = static_cast<TestThreadParams*>(param);

    p->mutex.lock();

    while (p->counter == 0) {
        p->cv.wait(&p->mutex);
    }

    p->mutex.unlock();
    return NULL;
}

static void* testThreadFunctionAutoLockWait(void* param) {
    TestThreadParams* p = static_cast<TestThreadParams*>(param);

    AutoLock lock(p->mutex);

    while (p->counter == 0) {
        p->cv.wait(&lock);
    }

    return NULL;
}


TEST(ConditionVariable, basicWaitSignal) {
    TestThreadParams p;

    TestThread testThread(testThreadFunctionBasicWait, &p);

    p.mutex.lock();
    p.counter++;
    p.cv.signal();
    p.mutex.unlock();

    testThread.join();

    EXPECT_EQ(p.counter, 1);
}

TEST(ConditionVariable, autoLockWaitSignal) {
    TestThreadParams p;

    TestThread testThread(testThreadFunctionAutoLockWait, &p);

    p.mutex.lock();
    p.counter++;
    p.cv.signal();
    p.mutex.unlock();

    testThread.join();

    EXPECT_EQ(p.counter, 1);
}

TEST(ConditionVariable, basicWaitBroadcast) {
    TestThreadParams p;

    TestThread testThreadA(testThreadFunctionBasicWait, &p);
    TestThread testThreadB(testThreadFunctionBasicWait, &p);

    p.mutex.lock();
    p.counter++;
    p.cv.broadcast();
    p.mutex.unlock();

    testThreadA.join();
    testThreadB.join();

    EXPECT_EQ(p.counter, 1);
}

TEST(ConditionVariable, waitBroadcast) {
    TestThreadParams p;
    const size_t kNumThreads = 20;

    std::vector<TestThread> testThreads;
    testThreads.reserve(kNumThreads);

    for (size_t n = 0; n < kNumThreads; ++n) {
        testThreads.emplace_back(testThreadFunctionBasicWait, &p);
    }

    p.mutex.lock();
    p.counter++;
    p.cv.broadcast();
    p.mutex.unlock();

    for (size_t i = 0; i < kNumThreads; ++i) {
        testThreads[i].join();
    }

    EXPECT_EQ(p.counter, 1);
}

}  // namespace base
}  // namespace android

