// Copyright (C) 2015 The Android Open Source Project
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

#include "android/utils/async.h"

#include "android/base/threads/Async.h"

using android::base::ThreadFlags;

bool asyncWithParam(async_function_t func, bool maskSignals) {
    return android::base::async(func,
                maskSignals ? ThreadFlags::MaskSignals : ThreadFlags::NoFlags);
}

bool async(async_function_t func) {
    return asyncWithParam(func, true);
}
