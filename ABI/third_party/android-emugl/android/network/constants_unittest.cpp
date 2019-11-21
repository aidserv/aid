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

#include "android/network/constants.h"

#include <gtest/gtest.h>

#define ARRAYLEN(x)  (sizeof(x)/sizeof(x[0]))

TEST(AndroidNetworkConstants, ParseSpeed) {
    static const struct {
        const char* input;
        bool expected_result;
        double expected_upload;
        double expected_download;
    } kData[] = {
        { nullptr, true, 0., 0. },
        { "", true, 0., 0. },
        { "unknown-speed", false, 0., 0. },
        { "abcd12345", false, 0., 0. },
        { "1000", true, 1e6, 1e6 },
        { "1000:20000", true, 1e6, 2e7 },
    };

    for (size_t n = 0; n < ARRAYLEN(kData); ++n) {
        double upload = 0.;
        double download = 0.;
        bool result = android_network_speed_parse(kData[n].input,
                                                  &upload, &download);
        EXPECT_EQ(kData[n].expected_result, result) << "# " << n;
        if (!result) {
            continue;
        }
        EXPECT_EQ(kData[n].expected_download, download) << "# " << n;
        EXPECT_EQ(kData[n].expected_upload, upload) << "# " << n;
    }
}

TEST(AndroidNetworkConstants, ParseSpeedWithName) {
    for (size_t n = 0; n < android_network_speeds_count; ++n) {
        const AndroidNetworkSpeed* entry = &android_network_speeds[n];
        double upload = 0.;
        double download = 0.;
        EXPECT_TRUE(android_network_speed_parse(entry->name,
                                                &upload,
                                                &download));
        EXPECT_EQ(entry->upload_bauds, upload) << "# " << n;
        EXPECT_EQ(entry->download_bauds, download) << "# " << n;
    }
}

TEST(AndroidNetworkConstants, ParseLatency) {
    static const struct {
        const char* input;
        bool expected_result;
        double expected_min_ms;
        double expected_max_ms;
    } kData[] = {
        { nullptr, true, 0., 0. },
        { "", true, 0., 0. },
        { "unknown-speed", false, 0., 0. },
        { "abcd12345", false, 0., 0. },
        { "1000", true, 1e3, 1e3 },
        { "1000:20000", true, 1e3, 2e4 },
    };

    for (size_t n = 0; n < ARRAYLEN(kData); ++n) {
        double min_ms = 0.;
        double max_ms = 0.;
        bool result = android_network_latency_parse(kData[n].input,
                                                    &min_ms, &max_ms);
        EXPECT_EQ(kData[n].expected_result, result) << "# " << n;
        if (!result) {
            continue;
        }
        EXPECT_EQ(kData[n].expected_max_ms, max_ms) << "# " << n;
        EXPECT_EQ(kData[n].expected_min_ms, min_ms) << "# " << n;
    }
}

TEST(AndroidNetworkConstants, ParseLatencyWithName) {
    for (size_t n = 0; n < android_network_latencies_count; ++n) {
        const AndroidNetworkLatency* entry = &android_network_latencies[n];
        double min_ms = 0.;
        double max_ms = 0.;
        EXPECT_TRUE(android_network_latency_parse(entry->name,
                                                  &min_ms,
                                                  &max_ms));
        EXPECT_EQ(entry->min_ms, min_ms) << "# " << n;
        EXPECT_EQ(entry->max_ms, max_ms) << "# " << n;
    }
}
