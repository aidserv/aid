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

#include "android/base/async/RecurrentTask.h"

#include "android/base/async/Looper.h"
#include "android/base/testing/TestLooper.h"
#include "android/base/testing/TestThread.h"

#include <gtest/gtest.h>

#include <memory>

using android::base::Looper;
using android::base::TestLooper;
using android::base::TestThread;
using android::base::RecurrentTask;
using std::unique_ptr;

class CountUpToN {
public:
    CountUpToN(Looper* looper, int maxCount)
        : mMaxCount(maxCount),
          mRecurrentTask(looper,
                         std::bind(&CountUpToN::countOne, this),
                         0),
          mPtrMayBeNull(&sideEffectVar) {}

    ~CountUpToN() {
        mPtrMayBeNull = nullptr;
    }

    RecurrentTask& task() {
        return mRecurrentTask;
    }

    void start() { mRecurrentTask.start(); }

    bool countOne() {

        blowUpIfDeleted();

        if (mCurCount >= mMaxCount) {
            return false;
        }
        ++mCurCount;
        return true;
    }

    void blowUpIfDeleted() {
        // Make sure this has a side effect.
        *mPtrMayBeNull = sideEffectVar;
    }

    int curCount() const { return mCurCount; }

private:
    int mMaxCount;
    RecurrentTask mRecurrentTask;
    int mCurCount = 0;

    // Designed to blow up if |countOne| is called after we've been deleted.
    volatile int* volatile mPtrMayBeNull;
    volatile int sideEffectVar;
};

TEST(RecurrentTaskTest, zeroTimes) {
    TestLooper looper;
    CountUpToN tasker(&looper, 0);
    tasker.start();

    EXPECT_TRUE(tasker.task().inFlight());
    EXPECT_EQ(0, tasker.curCount());
    looper.runWithTimeoutMs(500);
    EXPECT_FALSE(tasker.task().inFlight());
    EXPECT_EQ(0, tasker.curCount());
}

TEST(RecurrentTaskTest, fiveTimes) {
    TestLooper looper;
    CountUpToN tasker(&looper, 5);
    tasker.start();

    EXPECT_TRUE(tasker.task().inFlight());
    EXPECT_EQ(0, tasker.curCount());
    looper.runWithTimeoutMs(500);
    EXPECT_FALSE(tasker.task().inFlight());
    EXPECT_EQ(5, tasker.curCount());
}

TEST(RecurrentTaskTest, deleteStartedObject) {
    TestLooper looper;
    auto tasker = new CountUpToN(&looper, 5);
    tasker->start();
    delete tasker;

    // If |RecurrentTask| does not gracefully handle being deleted while active,
    // this will blow up.
    looper.runWithTimeoutMs(500);
}

static bool stopAndReturn(RecurrentTask* task, int* count) {
    task->stopAsync();
    // Let's not overflow and succeed flakily.
    if (*count < 10) {
        ++(*count);
    }
    return true;
}

TEST(RecurrentTaskTest, stopFromCallback) {
    TestLooper looper;
    int count = 0;
    RecurrentTask task(&looper,
                       [&task, &count]() {
                           return stopAndReturn(&task, &count);
                       },
                       0);
    task.start();
    EXPECT_TRUE(task.inFlight());
    looper.runWithTimeoutMs(50);
    EXPECT_EQ(1, count);
}

TEST(RecurrentTaskTest, stopAndWait) {
    TestLooper looper;
    int count = 0;
    RecurrentTask task(&looper,
                       [&count]() {
                           ++count;
                           return true;
                       },
                       0);
    task.start();
    EXPECT_TRUE(task.inFlight());

    TestThread runner([](void* param) -> void* {
        auto looper = (TestLooper*)param;
        EXPECT_EQ(EWOULDBLOCK,
                  looper->runWithDeadlineMs(Looper::kDurationInfinite));
        return nullptr;
    }, &looper);

    // If there's any but with the task's stop method, the test would hang in
    // the next call forever as looper won't be able to break out of its run()
    // call.
    task.stopAndWait();
    runner.join();

    // It's possible that the task won't run even once.
    EXPECT_GE(count, 0);
}
