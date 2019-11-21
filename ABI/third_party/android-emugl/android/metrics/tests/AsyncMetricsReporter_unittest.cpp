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

#include "android/metrics/AsyncMetricsReporter.h"

#include "android/base/Optional.h"
#include "android/base/StringView.h"
#include "android/metrics/proto/clientanalytics.pb.h"
#include "android/metrics/proto/studio_stats.pb.h"
#include "android/metrics/tests/MockMetricsWriter.h"
#include "android/utils/system.h"

#include <gtest/gtest.h>

using namespace android::base;
using namespace android::metrics;

static constexpr StringView kVersion = "version";
static constexpr StringView kFullVersion = "fullVersion";
static constexpr StringView kQemuVersion = "qemuVersion";

namespace {

class AsyncMetricsReporterTest : public ::testing::Test {
public:
    std::shared_ptr<MockMetricsWriter> mWriter{
            std::make_shared<MockMetricsWriter>()};
    Optional<AsyncMetricsReporter> mReporter;

    void SetUp() override {
        createReporter();
        ASSERT_TRUE((bool)mReporter);
    }

    void createReporter() {
        mReporter.emplace(mWriter, kVersion, kFullVersion, kQemuVersion);
    }

    void flushEvents() {
        mReporter.clear();
        createReporter();
    }
};

}  // namespace

TEST_F(AsyncMetricsReporterTest, isEnabled) {
    EXPECT_TRUE(mReporter->isReportingEnabled());
}

TEST_F(AsyncMetricsReporterTest, reportConditional) {
    const auto threadId = android_get_thread_id();
    mWriter->mOnWrite =
            [threadId](const android_studio::AndroidStudioEvent& asEvent,
                       wireless_android_play_playlog::LogEvent* logEvent) {
        EXPECT_NE(threadId, android_get_thread_id());
        EXPECT_FALSE(logEvent->has_source_extension());

        // Verify the fields AsyncMetricsReporter is supposed to fill in.
        EXPECT_TRUE(asEvent.has_product_details());
        EXPECT_STREQ(kVersion.c_str(),
                     asEvent.product_details().version().c_str());
        EXPECT_STREQ(kFullVersion.c_str(),
                     asEvent.product_details().build().c_str());

        EXPECT_TRUE(asEvent.has_emulator_details());
        EXPECT_STREQ(kQemuVersion.c_str(),
                     asEvent.emulator_details().core_version().c_str());
    };

    int reportCallbackCalls = 0;
    mReporter->reportConditional([threadId, &reportCallbackCalls](
            android_studio::AndroidStudioEvent* event) {
        EXPECT_TRUE(event != nullptr);
        EXPECT_NE(threadId, android_get_thread_id());
        ++reportCallbackCalls;
        return false;
    });

    flushEvents();
    EXPECT_EQ(1, reportCallbackCalls);
    EXPECT_EQ(0, mWriter->mWriteCallsCount);

    mReporter->reportConditional([threadId, &reportCallbackCalls](
            android_studio::AndroidStudioEvent* event) {
        EXPECT_TRUE(event != nullptr);
        EXPECT_NE(threadId, android_get_thread_id());
        ++reportCallbackCalls;
        return true;
    });

    flushEvents();
    EXPECT_EQ(2, reportCallbackCalls);
    EXPECT_EQ(1, mWriter->mWriteCallsCount);

    // Make sure nothing breaks on a null callback.
    mReporter->reportConditional({});
    flushEvents();
    EXPECT_EQ(1, mWriter->mWriteCallsCount);
}

TEST_F(AsyncMetricsReporterTest, finishPendingReports) {
    const int count = 10;
    int reported = 0;
    for (int i = 0; i < count; ++i) {
        mReporter->report([&reported](android_studio::AndroidStudioEvent*) {
            ++reported;
        });
        mReporter->reportConditional([&reported](android_studio::AndroidStudioEvent*) {
            ++reported;
            return true;
        });
        mReporter->reportConditional([&reported](android_studio::AndroidStudioEvent*) {
            ++reported;
            return false;
        });
    }
    mReporter->finishPendingReports();
    EXPECT_EQ(3 * count, reported);
}
