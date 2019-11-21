// Copyright (C) 2017 The Android Open Source Project
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

#include "android/metrics/MemoryUsageReporter.h"

#include "android/CommonReportedInfo.h"
#include "android/metrics/PeriodicReporter.h"

#include "android/metrics/proto/studio_stats.pb.h"
#include "android/utils/debug.h"

#define  D(...)  do {  if (VERBOSE_CHECK(init)) dprint(__VA_ARGS__); } while (0)

static const uint32_t kMemUsageMetricsReportIntervalMs = 60 * 1000;

namespace android {
namespace metrics {

using android::base::Lock;
using android::base::System;
using std::shared_ptr;

// static
MemoryUsageReporter::Ptr MemoryUsageReporter::create(
        android::base::Looper* looper,
        android::base::Looper::Duration checkIntervalMs) {
    auto inst = Ptr(new MemoryUsageReporter(looper, checkIntervalMs));
    return inst;
}

MemoryUsageReporter::MemoryUsageReporter(
        android::base::Looper* looper,
        android::base::Looper::Duration checkIntervalMs)
    : mLooper(looper),
      mCheckIntervalMs(checkIntervalMs),
      // We use raw pointer to |this| instead of a shared_ptr to avoid cicrular
      // ownership. mRecurrentTask promises to cancel any outstanding tasks when
      // it's destructed.
      mRecurrentTask(looper,
                     [this]() { return checkMemoryUsage(); },
                     mCheckIntervalMs) {

    // Also start up a periodic reporter that takes whatever is in the
    // current struct.
    using android::metrics::PeriodicReporter;
    PeriodicReporter::get().addTask(kMemUsageMetricsReportIntervalMs,
        [this](android_studio::AndroidStudioEvent* event) {
            android_studio::EmulatorMemoryUsage* memUsageProto =
                event->mutable_emulator_performance_stats()->add_memory_usage();

            android::base::AutoLock lock(mLock);
            System::MemUsage forMetrics = mCurrUsage;
            lock.unlock();

            memUsageProto->set_resident_memory(forMetrics.resident);
            memUsageProto->set_resident_memory_max(forMetrics.resident_max);
            memUsageProto->set_virtual_memory(forMetrics.virt);
            memUsageProto->set_virtual_memory_max(forMetrics.virt_max);
            memUsageProto->set_total_phys_memory(forMetrics.total_phys_memory);
            memUsageProto->set_total_page_file(forMetrics.total_page_file);

            {
                // TODO: Move performance stats setter to a separate location
                // once we are tracking more performance stats
                android_studio::EmulatorPerformanceStats forCommonInfo =
                    event->emulator_performance_stats();
                CommonReportedInfo::setPerformanceStats(
                    &forCommonInfo);
            }
            return true;
        });

    // Don't call start() here: start() launches a parallel task that calls
    // shared_from_this(), which needs at least one shared pointer owning
    // |this|. We can't guarantee that until the constructor call returns.
}

MemoryUsageReporter::~MemoryUsageReporter() {
    stop();
}

void MemoryUsageReporter::start() {
    mRecurrentTask.start();
}

void MemoryUsageReporter::stop() {
    mRecurrentTask.stopAndWait();
}

bool MemoryUsageReporter::checkMemoryUsage() {
    System::MemUsage usage = System::get()->getMemUsage();

    D("MemoryReport: Epoch: %lu, Res/ResMax/Virt/VirtMax: "
      "%" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64,
      time,
      usage.resident,
      usage.resident_max,
      usage.virt,
      usage.virt_max);

    android::base::AutoLock lock(mLock);
    mCurrUsage = usage;

    return true;
}

}  // namespace metrics
}  // namespace android

