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

#include "android/metrics/NullMetricsReporter.h"
#include "android/metrics/NullMetricsWriter.h"

#include "android/metrics/proto/clientanalytics.pb.h"
#include "android/metrics/proto/studio_stats.pb.h"
#include "android/metrics/tests/MockMetricsWriter.h"
#include "android/utils/system.h"

#include <gtest/gtest.h>

using namespace android::base;
using namespace android::metrics;

TEST(NullMetricsClassesTest, writer) {
    NullMetricsWriter writer;
    EXPECT_STREQ("", writer.sessionId().c_str());
    writer.write({}, {});  // does nothing, and there's no way to check it :(
}

TEST(NullMetricsClassesTest, reporter) {
    auto writer = std::make_shared<MockMetricsWriter>();
    writer->mOnWrite =
            [](const android_studio::AndroidStudioEvent& asEvent,
               wireless_android_play_playlog::LogEvent* logEvent) { FAIL(); };

    NullMetricsReporter reporter(writer);

    // Make sure nothing breaks on a null callback.
    reporter.reportConditional({});

    reporter.reportConditional([](android_studio::AndroidStudioEvent* event) {
        ADD_FAILURE();
        return false;
    });
    reporter.reportConditional([](android_studio::AndroidStudioEvent* event) {
        ADD_FAILURE();
        return true;
    });

    EXPECT_EQ(0, writer->mWriteCallsCount);
}
