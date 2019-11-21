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

#include "android/base/threads/ParallelTask.h"

#include "android/base/testing/TestLooper.h"
#include "android/base/system/System.h"

#include <gtest/gtest.h>

#include <atomic>
#include <functional>
#include <memory>

namespace {

using android::base::TestLooper;
using android::base::ParallelTask;
using std::unique_ptr;

struct Result {
    bool mWasJoined = false;
    int mExitStatus = 0;
};

class ParallelTaskTest : public testing::Test {
public:
    // set mCheckTimeoutMs to 10 to speed things up.
    ParallelTaskTest()
        : mParallelTask(&mLooper,
                        std::bind(&ParallelTaskTest::taskFunction,
                                  this,
                                  std::placeholders::_1),
                        std::bind(&ParallelTaskTest::taskDoneFunction,
                                  this,
                                  std::placeholders::_1),
                        10) {}

    void taskFunction(Result* outResult) {
        while (!mShouldRun) {
            android::base::System::get()->sleepMs(10);
        }
        outResult->mExitStatus = 42;
    }

    void taskDoneFunction(const Result& result) {
        mResult = result;
        mResult.mWasJoined = true;
    }

    void unblock() { mShouldRun = true; }

protected:
    TestLooper mLooper;
    ParallelTask<Result> mParallelTask;
    Result mResult;
    std::atomic_bool mShouldRun = {false};
};

TEST_F(ParallelTaskTest, start) {
    unblock();
    EXPECT_TRUE(mParallelTask.start());
    mLooper.runWithTimeoutMs(2000);

    EXPECT_FALSE(mParallelTask.inFlight());
    EXPECT_TRUE(mResult.mWasJoined);
    EXPECT_EQ(42, mResult.mExitStatus);
}

TEST_F(ParallelTaskTest, inFlight) {
    EXPECT_TRUE(mParallelTask.start());
    // The test will actually block here till timeout. Use a small value.
    mLooper.runWithTimeoutMs(100);
    EXPECT_FALSE(mResult.mWasJoined);
    EXPECT_TRUE(mParallelTask.inFlight());

    unblock();
    mLooper.runWithTimeoutMs(2000);
    EXPECT_FALSE(mParallelTask.inFlight());
    EXPECT_EQ(42, mResult.mExitStatus);
    EXPECT_TRUE(mResult.mWasJoined);
}

static bool parJoined = false;

static void setPar(int* outResult) {
    *outResult = 42;
}

static void onJoined(const int& outResult) {
    EXPECT_EQ(42, outResult);
    parJoined = true;
}

TEST(ParallelTaskFunctionTest, basic) {
    TestLooper looper;
    EXPECT_TRUE(android::base::runParallelTask<int>(&looper, &setPar, &onJoined, 10));
    EXPECT_FALSE(parJoined);
    looper.runWithTimeoutMs(2000);
    EXPECT_TRUE(parJoined);
}

}  // namespace
