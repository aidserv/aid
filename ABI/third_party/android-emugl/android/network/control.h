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

ANDROID_BEGIN_HEADER

// Change the global emulated network speed. |speed| is a string that can be
// recognized by android_network_speed_parse(). Returns true on success, or
// false otherwise (e.g. invalid format).
bool android_network_set_speed(const char* speed);

// Change the global emulator network latency. |latency| is a string that can
// be recognized by android_network_latency_parse(). Returns true on success,
// or false otherwise (e.g. invalid format).
bool android_network_set_latency(const char* latency);

ANDROID_END_HEADER
