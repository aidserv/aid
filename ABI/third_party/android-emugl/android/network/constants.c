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

#include <stdlib.h>
#include <string.h>

#define ARRAYLEN(x)  (sizeof(x)/sizeof(x[0]))

/* see http://en.wikipedia.org/wiki/List_of_device_bandwidths or a complete list */
const AndroidNetworkSpeed  android_network_speeds[] = {
#define DEFINE_NET_SPEED(name, display_name, upload, download, min_latency, max_latency) \
    { #name, display_name, (upload)*1000, (download)*1000 },
    ANDROID_NETWORK_LIST_MODES(DEFINE_NET_SPEED)
    { "full", "no limit", 0, 0 },
};
const size_t android_network_speeds_count = ARRAYLEN(android_network_speeds);

const char kAndroidNetworkDefaultSpeed[] = "full";

bool android_network_speed_parse(const char* speed,
                                 double* upload_bauds,
                                 double* download_bauds) {
    if (!speed || !speed[0]) {
        speed = kAndroidNetworkDefaultSpeed;
    }

    // First, check whether this is the well-known name
    int n;
    for (n = 0; n < android_network_speeds_count; ++n) {
        const AndroidNetworkSpeed* entry = &android_network_speeds[n];
        if (!strcmp(speed, entry->name)) {
            *upload_bauds = entry->upload_bauds;
            *download_bauds = entry->download_bauds;
            return true;
        }
    }

    // Second check for a number.
    char* end = NULL;
    double sp = strtod(speed, &end);
    if (end == speed) {
        return false;
    }

    *download_bauds = *upload_bauds = sp * 1000.;  // TODO(digit): Why 1000. ?
    if (*end == ':') {
        speed = end + 1;
        sp = strtod(speed, &end);
        if (end > speed) {
            *download_bauds = sp * 1000.;
        }
    }

    return true;
}

const AndroidNetworkLatency  android_network_latencies[] = {
#define DEFINE_NET_LATENCY(name, display_name, upload, download, min_latency, max_latency) \
    { #name, display_name, min_latency, max_latency },
    ANDROID_NETWORK_LIST_MODES(DEFINE_NET_LATENCY)
    { "none", "no latency", 0, 0 },
};
const size_t android_network_latencies_count =
        ARRAYLEN(android_network_latencies);

const char kAndroidNetworkDefaultLatency[] = "none";

bool android_network_latency_parse(const char* delay,
                                   double* min_delay_ms,
                                   double* max_delay_ms) {
    if (!delay || !delay[0]) {
        delay = kAndroidNetworkDefaultLatency;
    }

    int n;
    for (n = 0; n < android_network_latencies_count; ++n) {
        const AndroidNetworkLatency* entry = &android_network_latencies[n];
        if (!strcmp(entry->name, delay)) {
            *min_delay_ms = entry->min_ms;
            *max_delay_ms = entry->max_ms;
            return true;
        }
    }

    char* end = NULL;
    double sp = strtod(delay, &end);
    if (end == delay) {
        return false;
    }

    *min_delay_ms = *max_delay_ms = sp;
    if (*end == ':') {
        delay = end + 1;
        sp = strtod(delay, &end);
        if (end > delay) {
            *max_delay_ms = sp;
        }
    }

    return true;
}
