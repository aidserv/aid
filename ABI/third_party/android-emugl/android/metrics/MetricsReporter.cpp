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

#include "android/metrics/MetricsReporter.h"

#include "android/base/threads/Async.h"
#include "android/base/async/ThreadLooper.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/Uuid.h"
#include "android/cmdline-option.h"
#include "android/metrics/AsyncMetricsReporter.h"
#include "android/metrics/CrashMetricsReporting.h"
#include "android/metrics/FileMetricsWriter.h"
#include "android/metrics/MetricsPaths.h"
#include "android/metrics/NullMetricsReporter.h"
#include "android/metrics/StudioConfig.h"
#include "android/metrics/TextMetricsWriter.h"
#include "android/utils/debug.h"

#include "android/metrics/proto/clientanalytics.pb.h"
#include "android/metrics/proto/studio_stats.pb.h"


#include <stdio.h>
#include <type_traits>

using android::base::System;

namespace android {
namespace metrics {

namespace {

static base::LazyInstance<NullMetricsReporter> sNullInstance = {};

// A small class that ensures there's always an instance of metrics reporter
// ready to process a metrics write request.
// By default it's an instance of NullMetricsReporter() which discards all
// requests, but it can be reset to anything that implements MetricsReporter
// interface.
class ReporterHolder final {
public:
    ReporterHolder() : mPtr(sNullInstance.ptr()) {}

    void reset(MetricsReporter::Ptr newPtr) {
        if (newPtr) {
            mPtr = newPtr.release();
        } else {
            // Replace the current instance with a null one and delete the old
            // reporter (if it wasn't already a null reporter.
            MetricsReporter* other = sNullInstance.ptr();
            std::swap(mPtr, other);
            if (other != sNullInstance.ptr()) {
                delete other;
            }
        }
    }

    MetricsReporter& reporter() const { return *mPtr; }

private:
    MetricsReporter* mPtr;
};

static base::LazyInstance<ReporterHolder> sInstance = {};

}  // namespace

void MetricsReporter::start(const std::string& sessionId,
                            base::StringView emulatorVersion,
                            base::StringView emulatorFullVersion,
                            base::StringView qemuVersion) {
    MetricsWriter::Ptr writer;
    if (android_cmdLineOptions->metrics_to_console) {
        writer = TextMetricsWriter::create(base::StdioStream(stdout));
    } else if (android_cmdLineOptions->metrics_to_file != nullptr) {
        if (FILE* out = ::fopen(android_cmdLineOptions->metrics_to_file, "w")) {
            writer = TextMetricsWriter::create(
                    base::StdioStream(out, base::StdioStream::kOwner));
        } else {
            dwarning("Failed to open file '%s', disabling metrics reporting",
                     android_cmdLineOptions->metrics_to_file);
        }
    } else if (studio::getUserMetricsOptIn()) {
        writer = FileMetricsWriter::create(
                getSpoolDirectory(), sessionId,
                1000,  // record limit per single file
                base::ThreadLooper::get(),
                10 * 60 * 1000);  // time limit for a single file, ms
    }

    if (!writer) {
        sInstance->reset({});
    } else {
        sInstance->reset(Ptr(new AsyncMetricsReporter(
                writer, emulatorVersion, emulatorFullVersion, qemuVersion)));

        // Run the asynchronous cleanup/reporting job now.
        base::async([] {
            const auto sessions =
                    FileMetricsWriter::finalizeAbandonedSessionFiles(
                            getSpoolDirectory());
            reportCrashMetrics(get(), sessions);
        });
    }
}

void MetricsReporter::stop(MetricsStopReason reason) {
    sInstance->reporter().report(
                [reason](android_studio::AndroidStudioEvent* event) {
                    int crashCount = reason != METRICS_STOP_GRACEFUL ? 1 : 0;
                    event->mutable_emulator_details()->set_crashes(crashCount);
                });
    sInstance->reset({});
}

MetricsReporter& MetricsReporter::get() {
    return sInstance->reporter();
}

void MetricsReporter::report(Callback callback) {
    if (!callback) {
        return;
    }
    reportConditional([callback](android_studio::AndroidStudioEvent* event) {
        callback(event);
        return true;
    });
}

MetricsReporter::MetricsReporter(bool enabled, MetricsWriter::Ptr writer,
                                 base::StringView emulatorVersion,
                                 base::StringView emulatorFullVersion,
                                 base::StringView qemuVersion)
    : mWriter(std::move(writer)),
      mEnabled(enabled),
      mStartTimeMs(System::get()->getUnixTimeUs() / 1000),
      mEmulatorVersion(emulatorVersion),
      mEmulatorFullVersion(emulatorFullVersion),
      mQemuVersion(qemuVersion) {
    assert(mWriter);
}

MetricsReporter::~MetricsReporter() = default;

bool MetricsReporter::isReportingEnabled() const {
    return mEnabled;
}

const std::string& MetricsReporter::sessionId() const {
    // Protect this from unexpected changes in the MetricsWriter interface.
    static_assert(std::is_reference<decltype(mWriter->sessionId())>::value,
                  "MetricsWriter::sessionId() must return a reference");
    return mWriter->sessionId();
}

void MetricsReporter::sendToWriter(
        android_studio::AndroidStudioEvent* event) {
    wireless_android_play_playlog::LogEvent logEvent;

    const auto timeMs = System::get()->getUnixTimeUs() / 1000;
    logEvent.set_event_time_ms(timeMs);
    logEvent.set_event_uptime_ms(timeMs - mStartTimeMs);

    if (!event->has_kind()) {
        event->set_kind(android_studio::AndroidStudioEvent::EMULATOR_PING);
    }

    event->mutable_product_details()->set_product(
            android_studio::ProductDetails::EMULATOR);
    if (!mEmulatorVersion.empty()) {
        event->mutable_product_details()->set_version(mEmulatorVersion);
    }
    if (!mEmulatorFullVersion.empty()) {
        event->mutable_product_details()->set_build(mEmulatorFullVersion);
    }
    if (!mQemuVersion.empty()) {
        event->mutable_emulator_details()->set_core_version(mQemuVersion);
    }

    const auto times = System::get()->getProcessTimes();
    event->mutable_emulator_details()->set_system_time(times.systemMs);
    event->mutable_emulator_details()->set_user_time(times.userMs);
    event->mutable_emulator_details()->set_wall_time(times.wallClockMs);

    // Only set the session ID if it isn't set: some messages might be reported
    // on behalf of a different (e.g. crashed) session.
    if (!event->has_studio_session_id()) {
        event->set_studio_session_id(sessionId());
    }
    mWriter->write(*event, &logEvent);
}

}  // namespace metrics
}  // namespace android
