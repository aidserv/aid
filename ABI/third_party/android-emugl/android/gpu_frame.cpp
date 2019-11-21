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

#include "android/opengl/GpuFrameBridge.h"
#include "android/gpu_frame.h"

#include "android/base/Log.h"
#include "android/base/memory/LazyInstance.h"
#include "android/opengl/GpuFrameBridge.h"
#include "android/opengles.h"

// Standard values from Khronos.
#define GL_RGBA 0x1908
#define GL_UNSIGNED_BYTE 0x1401

using android::opengl::GpuFrameBridge;

static GpuFrameBridge* sBridge = NULL;

// Called from an EmuGL thread to transfer a new frame of the GPU display
// to the main loop.
static void onNewGpuFrame(void* opaque,
                          int width,
                          int height,
                          int ydir,
                          int format,
                          int type,
                          unsigned char* pixels) {
    DCHECK(ydir == -1);
    DCHECK(format == GL_RGBA);
    DCHECK(type == GL_UNSIGNED_BYTE);

    GpuFrameBridge* bridge = reinterpret_cast<GpuFrameBridge*>(opaque);
    bridge->postFrame(width, height, pixels);
}

void gpu_frame_set_post_callback(
        Looper* looper,
        void* context,
        void (*callback)(void*, int, int, const void*)) {
    DCHECK(!sBridge);

    sBridge = android::opengl::GpuFrameBridge::create(
            reinterpret_cast<android::base::Looper*>(looper),
            callback,
            context);
    CHECK(sBridge);

    android_setPostCallback(onNewGpuFrame, sBridge);
}
