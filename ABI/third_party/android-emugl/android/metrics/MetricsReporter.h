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

#pragma once

#include "android/base/Compiler.h"
#include "android/base/StringView.h"
#include "android/base/system/System.h"
#include "android/metrics/metrics.h"
#include "android/metrics/MetricsWriter.h"

#include <functional>
#include <memory>
#include <string>

namespace android_studio { class AndroidStudioEvent; }

namespace android {
namespace metrics {

//
// MetricsReporter - the main interface for metrics reporing in the emulator.
//
// To report a metric, call the report() method with a callback that will fill
// the passed |event| object with the metric values. E.g.
//
//  Screen screen = ...;
//  screen.DrawAFrame();
//  int frameTime = screen.GetFrameDrawingTime();
//  MetricsReporter::get().report([frameTime](AndroidStudioEvent* event) {
//      event->mutable_emulator_details()->set_frame_time(frameTime);
//  });
//
// This callback might be invoked either synchronously or asynchronously on a
// different thread, so it should capture all pieces of data it might need.
// It might even be never called at all - if metrics reporting is disabled -
// so make sure your code doesn't rely on that in any way.
//
// There is one advanced method, reportConditional(): it expects a different
// type of callback, one that returns |true| if it logged anything or |false| if
// it didn't and the metric message should be discarded. This is useful when
// metric reporting requires some long-running operation, and one doesn't want
// to run it on the same thread or to run it at all if metrics are disabled,
// but based on the outcome one might decide not to report it at all.
//
// Anyway, if even reportConditional() is not for you, you can call
// isReportingEnabled() to see if it makes sense at all to report the metrics.
//

class MetricsReporter {
    DISALLOW_COPY_ASSIGN_AND_MOVE(MetricsReporter);

public:
    using Ptr = std::unique_ptr<MetricsReporter>;
    using Callback = std::function<void (android_studio::AndroidStudioEvent*)>;
    using ConditionalCallback =
        std::function<bool (android_studio::AndroidStudioEvent*)>;

    static void start(const std::string& sessionId,
                      base::StringView emulatorVersion,
                      base::StringView emulatorFullVersion,
                      base::StringView qemuVersion);
    static void stop(MetricsStopReason reason);
    static MetricsReporter& get();

    virtual ~MetricsReporter();
    virtual void reportConditional(ConditionalCallback callback) = 0;
    // Wait for all pending reports to be finished.
    virtual void finishPendingReports() = 0;

    void report(Callback callback);
    // Checks if the metrics reporting is enabled for the current reporter
    // instance.
    bool isReportingEnabled() const;
    // Returns a unique identifier of the current emulator run (or session in
    // Android Studio terms).
    const std::string& sessionId() const;

protected:
    MetricsReporter(bool enabled, MetricsWriter::Ptr writer,
                    base::StringView emulatorVersion,
                    base::StringView emulatorFullVersion,
                    base::StringView qemuVersion);
    void sendToWriter(android_studio::AndroidStudioEvent* event);

private:
    const MetricsWriter::Ptr mWriter;
    const bool mEnabled = false;
    const base::System::Duration mStartTimeMs;
    const std::string mEmulatorVersion;
    const std::string mEmulatorFullVersion;
    const std::string mQemuVersion;
};

}  // namespace metrics
}  // namespace android
