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

#include "android/metrics/MetricsReporter.h"

#include "android/base/Optional.h"
#include "android/base/StringView.h"
#include "android/metrics/proto/clientanalytics.pb.h"
#include "android/metrics/proto/studio_stats.pb.h"
#include "android/metrics/tests/MockMetricsReporter.h"
#include "android/metrics/tests/MockMetricsWriter.h"
#include "android/utils/system.h"

#include <gtest/gtest.h>

using namespace android::base;
using namespace android::metrics;

static constexpr StringView kVersion = "version";
static constexpr StringView kFullVersion = "fullVersion";
static constexpr StringView kQemuVersion = "qemuVersion";
static constexpr StringView kSessionId = "session";

namespace {

class MetricsReporterTest : public ::testing::Test {
public:
    std::shared_ptr<MockMetricsWriter> mWriter{
            std::make_shared<MockMetricsWriter>(kSessionId)};
    Optional<MockMetricsReporter> mReporter;

    void createReporter(bool enabled = true,
                        StringView ver = kVersion,
                        StringView fullVer = kFullVersion,
                        StringView qemuVer = kQemuVersion) {
        mReporter.emplace(enabled, mWriter, ver, fullVer, qemuVer);
    }
};

}  // namespace

TEST_F(MetricsReporterTest, isEnabled) {
    createReporter(true);
    EXPECT_TRUE(mReporter->isReportingEnabled());

    createReporter(false);
    EXPECT_FALSE(mReporter->isReportingEnabled());
}

TEST_F(MetricsReporterTest, sessionId) {
    createReporter();
    EXPECT_STREQ(kSessionId.c_str(), mReporter->sessionId().c_str());
}

TEST_F(MetricsReporterTest, get) {
    // Make sure the default reporter has reporting disabled.
    EXPECT_FALSE(MetricsReporter::get().isReportingEnabled());
}

TEST_F(MetricsReporterTest, sendToWriter) {
    std::string sessionId = kSessionId;
    android_studio::AndroidStudioEvent::EventKind kind =
            android_studio::AndroidStudioEvent::EMULATOR_PING;
    bool expectVersions = true;
    mWriter->mOnWrite = [&kind, &sessionId, &expectVersions](
                        const android_studio::AndroidStudioEvent& asEvent,
                        wireless_android_play_playlog::LogEvent* logEvent) {
        EXPECT_TRUE(logEvent->has_event_time_ms());
        EXPECT_TRUE(logEvent->has_event_uptime_ms());
        EXPECT_FALSE(logEvent->has_source_extension());

        // Verify the fields MetricsReporter is supposed to fill in.
        EXPECT_TRUE(asEvent.has_studio_session_id());
        EXPECT_STREQ(sessionId.c_str(),
                     asEvent.studio_session_id().c_str());
        EXPECT_EQ(kind, asEvent.kind());

        EXPECT_TRUE(asEvent.has_product_details());
        EXPECT_TRUE(asEvent.has_emulator_details());

        EXPECT_EQ(android_studio::ProductDetails::EMULATOR,
                  asEvent.product_details().product());

        if (expectVersions) {
            EXPECT_STREQ(kVersion.c_str(),
                         asEvent.product_details().version().c_str());
            EXPECT_STREQ(kFullVersion.c_str(),
                         asEvent.product_details().build().c_str());
            EXPECT_STREQ(kQemuVersion.c_str(),
                         asEvent.emulator_details().core_version().c_str());
        } else {
            EXPECT_FALSE(asEvent.product_details().has_version());
            EXPECT_FALSE(asEvent.product_details().has_build());
            EXPECT_FALSE(asEvent.emulator_details().has_core_version());
        }
        EXPECT_TRUE(asEvent.emulator_details().has_system_time());
        EXPECT_TRUE(asEvent.emulator_details().has_user_time());
        EXPECT_TRUE(asEvent.emulator_details().has_wall_time());
    };

    createReporter();

    {
        // try an empty event
        android_studio::AndroidStudioEvent event;

        mReporter->sendToWriter(&event);

        EXPECT_EQ(1, mWriter->mWriteCallsCount);
    }
    {
        // now pre-fill event kind
        android_studio::AndroidStudioEvent event;
        kind = android_studio::AndroidStudioEvent::EMULATOR_HOST;
        event.set_kind(kind);

        mReporter->sendToWriter(&event);

        EXPECT_EQ(2, mWriter->mWriteCallsCount);

        // reset |kind| back
        kind = android_studio::AndroidStudioEvent::EMULATOR_PING;
    }
    {
        // pre-fill session ID
        android_studio::AndroidStudioEvent event;
        sessionId = "other session id";
        event.set_studio_session_id(sessionId);

        mReporter->sendToWriter(&event);

        EXPECT_EQ(3, mWriter->mWriteCallsCount);
        // reset |sessionId| back
        sessionId = kSessionId;
    }
    {
        // now create the reporter without versions and make sure they aren't
        // set in that case
        createReporter(true, {}, {}, {});
        expectVersions = false;
        android_studio::AndroidStudioEvent event;

        mReporter->sendToWriter(&event);

        EXPECT_EQ(4, mWriter->mWriteCallsCount);
        expectVersions = true;
    }
}

TEST_F(MetricsReporterTest, report) {
    createReporter();

    mReporter->mOnReportConditional = [](MetricsReporter::ConditionalCallback cb) {
        EXPECT_TRUE(bool(cb));
        android_studio::AndroidStudioEvent event;
        EXPECT_TRUE(cb(&event));
    };

    mReporter->report([](android_studio::AndroidStudioEvent* event) {});
    EXPECT_EQ(1, mReporter->mReportConditionalCallsCount);
    mReporter->report([](android_studio::AndroidStudioEvent* event) {});
    EXPECT_EQ(2, mReporter->mReportConditionalCallsCount);

    // empty callbacks don't even reach the reportConditional() override
    mReporter->report({});
    EXPECT_EQ(2, mReporter->mReportConditionalCallsCount);

    // reporter should never call writer from report() directly.
    EXPECT_EQ(0, mWriter->mWriteCallsCount);
}
