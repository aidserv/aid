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

#include "android/metrics/NullMetricsReporter.h"

#include "android/metrics/MetricsLogging.h"
#include "android/metrics/NullMetricsWriter.h"

namespace android {
namespace metrics {

NullMetricsReporter::NullMetricsReporter(MetricsWriter::Ptr writer)
    : MetricsReporter(false,
                      writer ? std::move(writer)
                             : MetricsWriter::Ptr(new NullMetricsWriter()),
                      {},
                      {},
                      {}) {
    D("created");
}

void NullMetricsReporter::reportConditional(ConditionalCallback callback) {
    D("ignoring");
}

}  // namespace metrics
}  // namespace android
