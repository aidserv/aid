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

#pragma once

#include "android/utils/compiler.h"
#include "android/utils/looper.h"

ANDROID_BEGIN_HEADER

// Initialize state to ensure that new GPU frame data is passed to the caller
// in the appropriate thread. |looper| is a Looper instance, |context| is an
// opaque handle passed to |callback| at runtime, which is a function called
// from the looper's thread whenever a new frame is available.
void gpu_frame_set_post_callback(
        Looper* looper,
        void* context,
        void (*callback)(void* context,
                         int width,
                         int height,
                         const void* pixels));

ANDROID_END_HEADER
