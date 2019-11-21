// Copyright 2016 The Android Open Source Project
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

#include "android/metrics/PeriodicReporter.h"

#include "android/base/Debug.h"
#include "android/base/Optional.h"
#include "android/base/StringView.h"
#include "android/base/testing/TestLooper.h"
#include "android/base/testing/TestSystem.h"
#include "android/base/testing/TestTempDir.h"
#include "android/base/Uuid.h"
#include "android/metrics/proto/clientanalytics.pb.h"
#include "android/metrics/proto/studio_stats.pb.h"
#include "android/metrics/tests/MockMetricsReporter.h"

#include <gtest/gtest.h>

#include <algorithm>

using namespace android::base;
using namespace android::metrics;

static constexpr StringView kSpoolDir = ".android/metrics/spool";

namespace {

class PeriodicReporterTest : public ::testing::Test {
public:
    TestSystem mSystem{"/", System::kProgramBitness, "/"};
    TestTempDir* mRoot = mSystem.getTempRoot();
    TestLooper mLooper;
    MockMetricsReporter mReporter;

    void SetUp() override {
        ASSERT_TRUE(mRoot != nullptr);
        mSystem.envSet("ANDROID_SDK_HOME", mRoot->pathString());

        PeriodicReporter::start(&mReporter, &mLooper);
    }

    void TearDown() {
        mReporter.mOnReportConditional = {};
        PeriodicReporter::stop();
    }
};

}  // namespace

TEST_F(PeriodicReporterTest, startStop) {
    // just make sure it doesn't crash if called multiple times
    PeriodicReporter::get();
    PeriodicReporter::stop();
    PeriodicReporter::stop();
    PeriodicReporter::get();
    PeriodicReporter::start(&mReporter, &mLooper);
}

TEST_F(PeriodicReporterTest, stop) {
    PeriodicReporter::get().addTask(
            100, [](android_studio::AndroidStudioEvent*) { return true; });

    EXPECT_EQ(0, mReporter.mFinishPendingReportsCallsCount);
    EXPECT_EQ(0, mReporter.mReportConditionalCallsCount);
    PeriodicReporter::stop();
    EXPECT_EQ(1, mReporter.mFinishPendingReportsCallsCount);
    EXPECT_EQ(1, mReporter.mReportConditionalCallsCount);
}

TEST_F(PeriodicReporterTest, addTask) {
    PeriodicReporter::get().addTask(
            100, [](android_studio::AndroidStudioEvent*) { return true; });
    EXPECT_EQ(1U, mLooper.timers().size());
    PeriodicReporter::get().addTask(
            1000, [](android_studio::AndroidStudioEvent*) { return true; });
    EXPECT_EQ(2U, mLooper.timers().size());
    PeriodicReporter::get().addTask(
            1000, [](android_studio::AndroidStudioEvent*) { return true; });
    EXPECT_EQ(2U, mLooper.timers().size());

    EXPECT_EQ(2U, mLooper.activeTimers().size());
}

TEST_F(PeriodicReporterTest, addCancelableTask) {
    auto task1 = PeriodicReporter::get().addCancelableTask(
            100, [](android_studio::AndroidStudioEvent*) { return true; });
    EXPECT_EQ(1U, mLooper.timers().size());
    auto task2 = PeriodicReporter::get().addCancelableTask(
            1000, [](android_studio::AndroidStudioEvent*) { return true; });
    EXPECT_EQ(2U, mLooper.timers().size());
    auto task3 = PeriodicReporter::get().addCancelableTask(
            1000, [](android_studio::AndroidStudioEvent*) { return true; });
    EXPECT_EQ(2U, mLooper.timers().size());

    EXPECT_EQ(2U, mLooper.activeTimers().size());

    task1.reset();
    EXPECT_EQ(1U, mLooper.activeTimers().size());
    task2.reset();
    EXPECT_EQ(1U, mLooper.activeTimers().size());
    task3.reset();
    EXPECT_EQ(0U, mLooper.activeTimers().size());
}

TEST_F(PeriodicReporterTest, addTaskMixed) {
    PeriodicReporter::get().addTask(
            100, [](android_studio::AndroidStudioEvent*) { return true; });
    EXPECT_EQ(1U, mLooper.timers().size());
    auto task = PeriodicReporter::get().addCancelableTask(
            100, [](android_studio::AndroidStudioEvent*) { return true; });
    EXPECT_EQ(1U, mLooper.timers().size());
    task.reset();
    EXPECT_EQ(1U, mLooper.timers().size());
}

TEST_F(PeriodicReporterTest, runTask) {
    mReporter.mOnReportConditional =
            [](MetricsReporter::ConditionalCallback cb) {
                ASSERT_TRUE(cb);
                android_studio::AndroidStudioEvent event;
                EXPECT_TRUE(cb(&event));
                EXPECT_STREQ("session", event.studio_session_id().c_str());
            };

    PeriodicReporter::get().addTask(
            100, [](android_studio::AndroidStudioEvent* event) {
                event->set_studio_session_id("session");
                return true;
            });

    mLooper.runOneIterationWithDeadlineMs(100);
    // make sure the task didn't run
    EXPECT_EQ(0, mReporter.mReportConditionalCallsCount);

    mSystem.setUnixTimeUs(100000);
    mLooper.runOneIterationWithDeadlineMs(100);
    // now it is should've been executed
    EXPECT_EQ(1, mReporter.mReportConditionalCallsCount);

    mSystem.setUnixTimeUs(199000);
    mLooper.runOneIterationWithDeadlineMs(199);
    // not executed for the second time yet
    EXPECT_EQ(1, mReporter.mReportConditionalCallsCount);

    mSystem.setUnixTimeUs(200000);
    mLooper.runOneIterationWithDeadlineMs(200);
    // second run
    EXPECT_EQ(2, mReporter.mReportConditionalCallsCount);
}

TEST_F(PeriodicReporterTest, runTaskNoReporting) {
    bool shouldReport1 = false;
    PeriodicReporter::get().addTask(
            100, [&shouldReport1](android_studio::AndroidStudioEvent* event) {
                return shouldReport1;
            });
    bool shouldReport2 = false;
    PeriodicReporter::get().addTask(
            100, [&shouldReport2](android_studio::AndroidStudioEvent* event) {
                return shouldReport2;
            });

    bool expectReport = false;
    mReporter.mOnReportConditional =
            [&expectReport](MetricsReporter::ConditionalCallback cb) {
                ASSERT_TRUE(cb);
                android_studio::AndroidStudioEvent event;
                EXPECT_EQ(expectReport, cb(&event));
            };

    mSystem.setUnixTimeUs(100000);
    shouldReport1 = false;
    shouldReport2 = false;
    expectReport = false;
    mLooper.runOneIterationWithDeadlineMs(100);
    EXPECT_EQ(1, mReporter.mReportConditionalCallsCount);

    mSystem.setUnixTimeUs(200000);
    shouldReport1 = true;
    shouldReport2 = false;
    expectReport = true;
    mLooper.runOneIterationWithDeadlineMs(200);
    EXPECT_EQ(2, mReporter.mReportConditionalCallsCount);

    mSystem.setUnixTimeUs(300000);
    shouldReport1 = false;
    shouldReport2 = true;
    expectReport = true;
    mLooper.runOneIterationWithDeadlineMs(300);
    EXPECT_EQ(3, mReporter.mReportConditionalCallsCount);

    mSystem.setUnixTimeUs(400000);
    shouldReport1 = true;
    shouldReport2 = true;
    expectReport = true;
    mLooper.runOneIterationWithDeadlineMs(400);
    EXPECT_EQ(4, mReporter.mReportConditionalCallsCount);
}

TEST_F(PeriodicReporterTest, runTaskWithCancellation) {
    mReporter.mOnReportConditional =
            [](MetricsReporter::ConditionalCallback cb) {
                ASSERT_TRUE(cb);
                android_studio::AndroidStudioEvent event;
                EXPECT_TRUE(cb(&event));
                EXPECT_STREQ("id", event.studio_session_id().c_str());
            };

    // Capture the task's token into the task itself and reset it in the middle
    // of reporting.
    PeriodicReporter::TaskToken task =
            PeriodicReporter::get().addCancelableTask(
                    100, [&task](android_studio::AndroidStudioEvent* event) {
                        event->set_studio_session_id("id");
                        task.reset();
                        return true;
                    });

    mSystem.setUnixTimeUs(100000);
    mLooper.runOneIterationWithDeadlineMs(100);
    // check that we ran the task at first...
    EXPECT_EQ(1, mReporter.mReportConditionalCallsCount);

    // ...but there's no task to run anymore
    EXPECT_FALSE(task);
    mSystem.setUnixTimeUs(200000);
    mLooper.runOneIterationWithDeadlineMs(200);
    EXPECT_EQ(1, mReporter.mReportConditionalCallsCount);
}

TEST_F(PeriodicReporterTest, taskTokenOutlivesReporter) {
    mReporter.mOnReportConditional =
            [](MetricsReporter::ConditionalCallback cb) {
                ASSERT_TRUE(cb);
                android_studio::AndroidStudioEvent event;
                EXPECT_TRUE(cb(&event));
            };

    auto task = PeriodicReporter::get().addCancelableTask(
            100,
            [](android_studio::AndroidStudioEvent* event) { return true; });

    mSystem.setUnixTimeUs(100000);
    mLooper.runOneIterationWithDeadlineMs(100);
    // check that we ran the task at first...
    EXPECT_EQ(1, mReporter.mReportConditionalCallsCount);

    // now stop the reporter and make sure it doesn't crash neither during the
    // reset nor when we try running tasks again or resetting the token
    PeriodicReporter::stop();
    EXPECT_TRUE(task);
    EXPECT_EQ(2, mReporter.mReportConditionalCallsCount);

    mSystem.setUnixTimeUs(200000);
    mLooper.runOneIterationWithDeadlineMs(200);
    EXPECT_EQ(2, mReporter.mReportConditionalCallsCount);

    task.reset();
}
