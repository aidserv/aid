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

#include "android/metrics/PeriodicReporter.h"

#include "android/base/memory/LazyInstance.h"

#include <iterator>
#include <type_traits>
#include <utility>

using android::base::AutoLock;
using android::base::LazyInstance;
using android::base::Looper;
using android::base::System;

namespace android {
namespace metrics {

// An instance holder object: it manages the creation of the default reporter
// instance.
struct PeriodicReporter::InstanceHolder {
    PeriodicReporter::Ptr reporter;

    InstanceHolder() : reporter(new PeriodicReporter()) {}
};

static LazyInstance<PeriodicReporter::InstanceHolder> sReporter = {};

void PeriodicReporter::start(MetricsReporter* metricsReporter, Looper* looper) {
    sReporter->reporter->startImpl(metricsReporter, looper);
}

void PeriodicReporter::stop() {
    // Make sure we create a new instance of periodic reporter here:
    // this way we expire all weak pointers in TaskGuards and they won't
    // try erasing from a list using invalid stale iterators.
    sReporter->reporter.reset(new PeriodicReporter());
}

PeriodicReporter& PeriodicReporter::get() {
    assert(sReporter->reporter.get());
    return *sReporter->reporter;
}

PeriodicReporter::PeriodicReporter() = default;

PeriodicReporter::~PeriodicReporter() {
    {
        AutoLock lock(mLock);
        // Run all scheduled tasks once to make sure we report all periodic
        // activities before exiting.
        for (auto& pair : mPeriodDataByPeriod) {
            lock.unlock();
            PerPeriodData* const data = &pair.second;
            reportForPerPeriodData(data);
            // We need the lock when clearing out the task, as it is possible
            // that we are removing it at the same time.
            lock.lock();
            data->task.clear();
        }
    }

    // Check in case stop() is called without start()
    if (mMetricsReporter) {
        mMetricsReporter->finishPendingReports();
    }

    AutoLock lock(mLock);
    mPeriodDataByPeriod.clear();
}

void PeriodicReporter::addTask(System::Duration periodMs, Callback callback) {
    AutoLock lock(mLock);
    addTaskInternalNoLock(periodMs, std::move(callback));
}

PeriodicReporter::TaskToken PeriodicReporter::addCancelableTask(
        System::Duration periodMs,
        PeriodicReporter::Callback callback) {
    // Capture a weak pointer to |this| into the task guard to make sure it can
    // outlive |this| and won't try to call into a dangling pointer.
    const auto weakThis = WeakPtr(shared_from_this());

    AutoLock lock(mLock);
    const auto iter = addTaskInternalNoLock(periodMs, std::move(callback));
    lock.unlock();

    return std::make_shared<TaskGuard>([periodMs, iter, weakThis]() {
        if (const auto sharedThis = weakThis.lock()) {
            sharedThis->removeTask(periodMs, iter);
        }
    });
}

void PeriodicReporter::startImpl(MetricsReporter* metricsReporter,
                                 Looper* looper) {
    AutoLock lock(mLock);

    assert(!mMetricsReporter);
    assert(!mLooper);

    assert(metricsReporter);
    assert(looper);

    mMetricsReporter = metricsReporter;
    mLooper = looper;

    for (auto& periodAndData : mPeriodDataByPeriod) {
        assert(!periodAndData.second.task);

        createPerPeriodTimerNoLock(&periodAndData.second, periodAndData.first);
    }
}

void PeriodicReporter::reportForPerPeriodData(
        PeriodicReporter::PerPeriodData* data) {
    static_assert(!std::is_reference<decltype(data)>::value,
                  "|data| must not be a reference: gcc 4.6 used to have a bug "
                  "where capturing a reference by value resulted in silent bad "
                  "code generation");

    mMetricsReporter->reportConditional(
            [this, data](android_studio::AndroidStudioEvent* event) {
                AutoLock lock(mLock);
                bool result = false;
                // Callback may erase itself from the list, deal with that by
                // using raw iterators in the loop.
                auto itCallback = data->callbacks.begin();
                while (itCallback != data->callbacks.end()) {
                    const auto itCurrentCallback = itCallback++;
                    // As task may try removing itself, unlock the object
                    // temporarily here.
                    lock.unlock();
                    result |= (*itCurrentCallback)(event);
                    lock.lock();
                }
                return result;
            });
}

void PeriodicReporter::createPerPeriodTimerNoLock(PerPeriodData* const data,
                                                  System::Duration periodMs) {
    assert(mLooper);
    data->task.emplace(mLooper,
                       [this, data]() {
                           reportForPerPeriodData(data);
                           return true;
                       },
                       periodMs);
    data->task->start();
}

PeriodicReporter::CallbackList::iterator
PeriodicReporter::addTaskInternalNoLock(System::Duration periodMs,
                                        PeriodicReporter::Callback callback) {
    PerPeriodData& data = mPeriodDataByPeriod[periodMs];
    data.callbacks.push_back(std::move(callback));
    if (mLooper && (!data.task || data.callbacks.size() == 1)) {
        createPerPeriodTimerNoLock(&data, periodMs);
    }

    const auto myIt = std::prev(data.callbacks.end());
    return myIt;
}

void PeriodicReporter::removeTask(System::Duration periodMs,
                                  CallbackList::iterator iter) {
    AutoLock lock(mLock);

    // There is a corner case where we try to remove a
    // task just as we are being cleaned up
    auto it = mPeriodDataByPeriod.find(periodMs);
    if (it == mPeriodDataByPeriod.end()) {
      return;
    }

    PerPeriodData& data = it->second;
    data.callbacks.erase(iter);
    if (data.task && data.callbacks.empty()) {
        data.task->stopAsync();
    }
}

}  // namespace metrics
}  // namespace android
