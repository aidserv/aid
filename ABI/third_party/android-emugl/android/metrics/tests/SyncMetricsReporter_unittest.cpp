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

#include "android/metrics/SyncMetricsReporter.h"

#include "android/metrics/proto/clientanalytics.pb.h"
#include "android/metrics/tests/MockMetricsWriter.h"
#include "android/utils/system.h"

#include <gtest/gtest.h>

using namespace android::base;
using namespace android::metrics;

namespace {

class SyncMetricsReporterTest : public ::testing::Test {
public:
    std::shared_ptr<MockMetricsWriter> mWriter{
            std::make_shared<MockMetricsWriter>()};
    SyncMetricsReporter mReporter{mWriter};
};

}  // namespace

TEST_F(SyncMetricsReporterTest, isEnabled) {
    EXPECT_TRUE(mReporter.isReportingEnabled());
}

TEST_F(SyncMetricsReporterTest, reportConditional) {
    const auto threadId = android_get_thread_id();
    mWriter->mOnWrite =
            [threadId](const android_studio::AndroidStudioEvent& asEvent,
                       wireless_android_play_playlog::LogEvent* logEvent) {
                EXPECT_EQ(threadId, android_get_thread_id());
                EXPECT_FALSE(logEvent->has_source_extension());
            };

    int reportCallbackCalls = 0;
    mReporter.reportConditional([threadId, &reportCallbackCalls](
            android_studio::AndroidStudioEvent* event) {
        EXPECT_TRUE(event != nullptr);
        EXPECT_EQ(threadId, android_get_thread_id());
        ++reportCallbackCalls;
        return false;
    });

    EXPECT_EQ(1, reportCallbackCalls);
    EXPECT_EQ(0, mWriter->mWriteCallsCount);

    mReporter.reportConditional([threadId, &reportCallbackCalls](
            android_studio::AndroidStudioEvent* event) {
        EXPECT_TRUE(event != nullptr);
        EXPECT_EQ(threadId, android_get_thread_id());
        ++reportCallbackCalls;
        return true;
    });

    EXPECT_EQ(2, reportCallbackCalls);
    EXPECT_EQ(1, mWriter->mWriteCallsCount);

    // Make sure nothing breaks on a null callback.
    mReporter.reportConditional({});
    EXPECT_EQ(1, mWriter->mWriteCallsCount);
}
