// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/metrics/CrashMetricsReporting.h"

#include "android/metrics/proto/studio_stats.pb.h"

#include <string>

namespace android {
namespace metrics {

void reportCrashMetrics(MetricsReporter& reporter,
                        const MetricsWriter::AbandonedSessions& sessions) {
    // Write down the crashed sessions as separate messages.
    for (const std::string& sessionId : sessions) {
        reporter.report([sessionId](android_studio::AndroidStudioEvent* event) {
            event->set_studio_session_id(sessionId);
            event->mutable_emulator_details()->set_crashes(1);
        });
    }
}

}  // namespace metrics
}  // namespace android
