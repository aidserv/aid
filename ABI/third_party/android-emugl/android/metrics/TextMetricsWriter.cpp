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

#include "android/metrics/TextMetricsWriter.h"

#include "android/metrics/MetricsLogging.h"

#include "android/metrics/proto/clientanalytics.pb.h"
#include "android/metrics/proto/studio_stats.pb.h"

#include <utility>

namespace android {
namespace metrics {

TextMetricsWriter::Ptr TextMetricsWriter::create(
        base::StdioStream&& outStream) {
    return Ptr(new TextMetricsWriter(std::move(outStream)));
}

TextMetricsWriter::TextMetricsWriter(base::StdioStream&& outStream)
    : MetricsWriter({}), mOutStream(std::move(outStream)) {}

void TextMetricsWriter::write(
        const android_studio::AndroidStudioEvent& asEvent,
        wireless_android_play_playlog::LogEvent* logEvent) {
    D("writing a log event with uptime %ld ms", logEvent->event_uptime_ms());

    fprintf(mOutStream.get(), "text_metrics: { %s }\n",
            asEvent.ShortDebugString().c_str());
    fflush(mOutStream.get());
}

}  // namespace metrics
}  // namespace android
