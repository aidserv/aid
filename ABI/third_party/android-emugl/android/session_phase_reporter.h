// Copyright 2017 The Android Open Source Project
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

#include "android/avd/info.h"

ANDROID_BEGIN_HEADER

typedef enum {
    ANDROID_SESSION_PHASE_UNKNOWN = 0,
    ANDROID_SESSION_PHASE_LAUNCHER = 1,
    ANDROID_SESSION_PHASE_PARSEOPTIONS = 2,
    ANDROID_SESSION_PHASE_INITGENERAL = 3,
    ANDROID_SESSION_PHASE_INITGPU = 4,
    ANDROID_SESSION_PHASE_INITACCEL = 5,
    ANDROID_SESSION_PHASE_RUNNING = 6,
    ANDROID_SESSION_PHASE_EXIT = 7,
    ANDROID_SESSION_PHASE_MAX = 8,
} AndroidSessionPhase;

void android_report_session_phase(AndroidSessionPhase phase);
AndroidSessionPhase android_get_session_phase();

ANDROID_END_HEADER
