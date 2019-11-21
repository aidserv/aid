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

ANDROID_BEGIN_HEADER

// A collection of network-related global variables.

// TODO: Change this to real function for proper mock support during
//       unit-testing of client-code that use these. For now, this is
//       really QEMU-only, so that's low priority.

// Emulated network up/down speeds, expressed in bits/seconds.
extern double android_net_upload_speed;
extern double android_net_download_speed;

// Emulated network min-max latency, expressed in ms.
extern int android_net_min_latency;
extern int android_net_max_latency;

// Global flag, when true, network is disabled.
extern int android_net_disable;

ANDROID_END_HEADER
