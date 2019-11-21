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

#include "android/session_phase_reporter.h"

#include "android/CommonReportedInfo.h"
#include "android/metrics/proto/studio_stats.pb.h"

static AndroidSessionPhase sCurrentSessionPhase;

void android_report_session_phase(
        AndroidSessionPhase phase) {
    sCurrentSessionPhase = phase;
    android::CommonReportedInfo::setSessionPhase(phase);
    // TODO: Also report session phase to metrics. But should we?
}

AndroidSessionPhase android_get_session_phase() {
    return sCurrentSessionPhase;
}
