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

#include "android/metrics/SyncMetricsReporter.h"

#include "android/metrics/MetricsLogging.h"
#include "android/metrics/proto/studio_stats.pb.h"

namespace android {
namespace metrics {

SyncMetricsReporter::SyncMetricsReporter(MetricsWriter::Ptr writer)
    : MetricsReporter(true, std::move(writer), {}, {}, {}) {
    D("created");
}

void SyncMetricsReporter::reportConditional(ConditionalCallback callback) {
    if (callback) {
        D("executing a reporting callback");
        android_studio::AndroidStudioEvent event;
        if (callback(&event)) {
            sendToWriter(&event);
        }
    }
}

}  // namespace metrics
}  // namespace android
