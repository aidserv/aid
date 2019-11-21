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

#include "android/metrics/tests/MockMetricsReporter.h"

#include "android/metrics/NullMetricsWriter.h"

namespace android {
namespace metrics {

MockMetricsReporter::MockMetricsReporter()
    : MetricsReporter(false,
                      MetricsWriter::Ptr(new NullMetricsWriter()),
                      {},
                      {},
                      {}) {}

MockMetricsReporter::MockMetricsReporter(bool enabled,
                                         MetricsWriter::Ptr writer,
                                         base::StringView emulatorVersion,
                                         base::StringView emulatorFullVersion,
                                         base::StringView qemuVersion)
    : MetricsReporter(enabled,
                      writer,
                      emulatorVersion,
                      emulatorFullVersion,
                      qemuVersion) {}

void MockMetricsReporter::reportConditional(
        MetricsReporter::ConditionalCallback callback) {
    ++mReportConditionalCallsCount;
    if (mOnReportConditional) {
        mOnReportConditional(callback);
    }
}

void MockMetricsReporter::finishPendingReports() {
    ++mFinishPendingReportsCallsCount;
    if (mOnFinishPendingReports) {
        mOnFinishPendingReports();
    }
}

}  // namespace metrics
}  // namespace android
