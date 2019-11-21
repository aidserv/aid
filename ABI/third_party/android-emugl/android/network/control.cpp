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

bool android_network_set_speed(const char* speed) {
    double upload = 0., download = 0.;
    if (!android_network_speed_parse(speed, &upload, &download)) {
        return false;
    }

    android_net_upload_speed = upload;
    android_net_download_speed = download;

    return true;
}

bool android_network_set_latency(const char* latency) {
    double min_delay_ms = 0., max_delay_ms = 0.;
    if (!android_network_latency_parse(latency, &min_delay_ms, &max_delay_ms)) {
        return false;
    }
    android_net_min_latency = (int)min_delay_ms;
    android_net_max_latency = (int)max_delay_ms;

    return true;
}
