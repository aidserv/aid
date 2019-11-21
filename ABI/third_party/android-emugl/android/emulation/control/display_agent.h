// Copyright 2015 The Android Open Source Project
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

#include <stdint.h>

ANDROID_BEGIN_HEADER

// This is an interface for Qemu display interaction

// Called when display is updated
// |opaque| - a user-supplied value to pass back
// |x|, |y|, |w|, |h| - boundaries of the updated area
typedef void (*AndroidDisplayUpdateCallback)(void* opaque, int x, int y,
                                             int w, int h);

typedef struct QAndroidDisplayAgent {
    // Fills in frame buffer parameters into the passed variables
    // |w|, |h| - width and height
    // |lineSize| - bytes per line
    // |bytesPerPixel| - bytes per pixel
    // |frameBufferData| - pointer to the raw frame buffer data
    void (*getFrameBuffer)(int* w, int* h, int* lineSize, int* bytesPerPixel,
                           uint8_t** frameBufferData);

    // Registers a callback which is called every time frame buffer content
    // is updated
    // |callback| - a callback to call
    // |opaque| - user data to pass to the callback
    void (*registerUpdateListener)(AndroidDisplayUpdateCallback callback,
                                   void* opaque);
} QAndroidDisplayAgent;

ANDROID_END_HEADER
