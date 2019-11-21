// Copyright 2016 The Android Open Source Project
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

#include "android/network/control.h"

#include "android/network/constants.h"
#include "android/network/globals.h"

#include <gtest/gtest.h>

// NOTE: Until we have a better API, access the global variables directly
//       to ensure they were updated accordingly.

TEST(AndroidNetworkControl, SetSpeed) {
    static const struct {
        const char* speed;
        bool result;
        double expected_upload;
        double expected_download;
    } kData[] = {
        { "", true, 0., 0. },
        { "1000", true, 1e6, 1e6 },
        { "1000:20000", true, 1e6, 2e7 },
        { "unknown-speed", false, 0., 0. },
    };
    for (const auto& item : kData) {
        // Set invalid values before the call.
        android_net_upload_speed = -100.0;
        android_net_download_speed = -200.0;

        EXPECT_EQ(item.result,
                  android_network_set_speed(item.speed))
                << "[" << item.speed << "]";
        if (item.result) {
            EXPECT_EQ(item.expected_upload, android_net_upload_speed) << item.speed;
            EXPECT_EQ(item.expected_download, android_net_download_speed) << item.speed;
        } else {
            EXPECT_EQ(-100, android_net_upload_speed);
            EXPECT_EQ(-200, android_net_download_speed);
        }
    }
}

TEST(AndroidNetworkControl, SetLatency) {
    static const struct {
        const char* delay;
        bool result;
        int expected_min_ms;
        int expected_max_ms;
    } kData[] = {
        { "", true, 0, 0 },
        { "1000", true, 1000, 1000 },
        { "1000:20000", true, 1000, 20000 },
        { "unknown-latency", false, 0, 0 },
    };
    for (const auto& item : kData) {
        android_net_min_latency = -100;
        android_net_max_latency = -200;

        EXPECT_EQ(item.result,
                  android_network_set_latency(item.delay))
            << "[" << item.delay << "]";

        if (item.result) {
            EXPECT_EQ(item.expected_min_ms, android_net_min_latency);
            EXPECT_EQ(item.expected_max_ms, android_net_max_latency);
        } else {
            EXPECT_EQ(-100, android_net_min_latency);
            EXPECT_EQ(-200, android_net_max_latency);
        }
    }
}
