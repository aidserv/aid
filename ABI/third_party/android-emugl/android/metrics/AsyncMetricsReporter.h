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

#include "android/base/synchronization/MessageChannel.h"
#include "android/base/threads/FunctorThread.h"
#include "android/metrics/MetricsReporter.h"

namespace android {
namespace metrics {

//
// AsyncMetricsReporter - an implementation of MetricsReporter that sends
// metrics messages asynchronously in a dedicated thread.
//
// Note: it is safe to have a separate worker thread that modified protobuf
//  message object: this is actually the only thread that modifies it. We create
//  an object in a worker thread, invoke the callback to fill its fields on the
//  same thread, and pass it to the writer here as well. This means there are
//  no multithreading issues to deal with.
//

class AsyncMetricsReporter final : public MetricsReporter
{
public:
    AsyncMetricsReporter(MetricsWriter::Ptr writer,
                         base::StringView emulatorVersion,
                         base::StringView emulatorFullVersion,
                         base::StringView qemuVersion);
    ~AsyncMetricsReporter() override;

    void reportConditional(ConditionalCallback callback) override;
    void finishPendingReports() override;

private:
    void worker();

    android::base::FunctorThread mWorkerThread;

    // A message channel for the pending callbacks. Empty callback
    // (ConditionalCallback{}) is a stop signal.
    // Note: reserved size of 100 should be enough, if we ever get blocked on
    // a full message channel then it's way too many messages being reported
    // for a normal emulator state.
    android::base::MessageChannel<ConditionalCallback, 100> mCallbackQueue;
};

}  // namespace metrics
}  // namespace android
