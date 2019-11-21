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

#include "android/metrics/MetricsWriter.h"

#include <functional>
#include <string>

namespace android {
namespace metrics {

class MockMetricsWriter final : public MetricsWriter {
public:
    using OnWrite =
            std::function<void(const android_studio::AndroidStudioEvent&,
                               wireless_android_play_playlog::LogEvent*)>;

    MockMetricsWriter(const std::string& sessionId = {});

    void write(const android_studio::AndroidStudioEvent& asEvent,
               wireless_android_play_playlog::LogEvent* logEvent) override;

    int mWriteCallsCount = 0;
    OnWrite mOnWrite;
};

}  // namespace metrics
}  // namespace android
