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

#pragma once

#include "android/utils/compiler.h"

#include <stdbool.h>
#include <stddef.h>

ANDROID_BEGIN_HEADER

// A convenience macro used to list all known network telephone interface.
// X() must be a macro that takes 6 parameters which are:
// - unique name (as used in the Android console).
// - display name.
// - upload rate in kB/s (*not* bits/s nor kiB/s).
// - download rate in kB/s.
// - minimum latency in milliseconds.
// - maximum latency in milliseconds.
// NOTE: Most of the latency numbers are completely made up and the
// upload/download rate come from the following wikipedia page:
//  http://en.wikipedia.org/wiki/List_of_device_bandwidths
#define ANDROID_NETWORK_LIST_MODES(X) \
    X(gsm,   "GSM/CD",        14.4,     14.4, 150, 550) \
    X(hscsd, "HSCSD",         14.4,     57.6,  80, 400) \
    X(gprs,  "GPRS",          28.8,     57.6,  35, 200) \
    X(umts,  "UMTS/3G",      384.0,    384.0,  35, 200) \
    X(edge,  "EDGE/EGPRS",   473.6,    473.6,  80, 400) \
    X(hsdpa, "HSDPA",       5760.0,  13980.0,   0,   0) \
    X(lte,   "LTE",        58000.0, 173000.0,   0,   0) \
    X(evdo,  "EVDO",       75000.0, 280000.0,   0,   0) \

// Describes one of the supported emulated network bandwith limits.
// |name| is the name as used in the Android console (e.g. 'gsm')
// |display_name| is the corresponding display name (e.g. 'GSM/CD')
// |upload_bauds| is the max upload speed in bits/s. 0 for unlimited.
// |download_bauds| is the max download speed in bits/s. 0 for unlimited.
typedef struct {
    const char*  name;
    const char*  display_name;
    double       upload_bauds;
    double       download_bauds;
} AndroidNetworkSpeed;

// Array of AndroidNetworkSpeed instances supported by AndroidEmu.
// There are |android_network_speeds_count| items in the array.
extern const AndroidNetworkSpeed android_network_speeds[];

// Number of items in |android_network_speeds|.
extern const size_t android_network_speeds_count;

// Default network speed setting, as a string. Used by console code.
extern const char kAndroidNetworkDefaultSpeed[];

// Extract network bandwidth limits from a string. |speed| is a textual
// description of the speed. The following formats are accepted:
//
//   - nullptr or <empty> for the default speed (kAndroidNetworkDefaultSpeed).
//   - <name>, where <name> is listed in android_network_speeds[].
//   - <decimal>, for both upload and download speed in KB/s (*not* KiB/s).
//   - <upload-decimal>:<download-decimal> for distinct upload/download speeds.
//
// Return true on success, false otherwise (bad format).
extern bool android_network_speed_parse(const char* speed,
                                        double* upload_bauds,
                                        double* download_bauds);

// Describes one of the supported network latency limits.
// |display| is the name as used in the Android console (e.g. 'edge').
// |display_name| is the corresponding display name (e.g. 'EDGE/EGPRS').
// |min_ms| is the minimal latency in milliseconds.
// |max_ms| is the maximal latency in milliseconds. 0 for unlimited.
typedef struct {
    const char*  name;
    const char*  display_name;
    int          min_ms;
    int          max_ms;
} AndroidNetworkLatency;

// Array of supported AndroidNetworkLatency entries.
extern const AndroidNetworkLatency android_network_latencies[];

// Number of entries in android_network_latencies.
extern const size_t android_network_latencies_count;

// Default network latency setting, as a string. Used by console code.
extern const char kAndroidNetworkDefaultLatency[];

// Extract network latency in milliseconds from a string. |delay| is
// the latency description, in one of the following formats:
//
// - nullptr or empty for the default latency (kAndroidNetworkDefaultLatency).
// - <name>, where <name> is listed in android_network_latencies[].
// - <decimal> for both minimum and maximum latencies in milliseconds.
// - <min-decimal>:<max-decimal> for distinct latency limits.
//
// Return true on success, false on failulre (bad format).
extern bool android_network_latency_parse(const char* delay,
                                          double* min_delay_ms,
                                          double* max_delay_ms);

ANDROID_END_HEADER
