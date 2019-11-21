// Copyright (C) 2016 The Android Open Source Project
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

// #include "android/base/Profiler.h"
#include "android/opengl/emugl_config.h"
#if 0
typedef enum SelectedRenderer {
    SELECTED_RENDERER_UNKNOWN = 0,
    SELECTED_RENDERER_HOST = 1,
    SELECTED_RENDERER_OFF = 2,
    SELECTED_RENDERER_GUEST = 3,
    SELECTED_RENDERER_MESA = 4,
    SELECTED_RENDERER_SWIFTSHADER = 5,
    SELECTED_RENDERER_ANGLE = 6, // ANGLE D3D11 with D3D9 fallback
    SELECTED_RENDERER_ANGLE9 = 7, // ANGLE forced to D3D9
    SELECTED_RENDERER_ERROR = 255,
}SelectedRenderer;
#endif


namespace emugl {

    // Set and get API version of system image.
    void setAvdInfo(bool isPhone, int apiLevel);
    void getAvdInfo(bool* isPhone, int* apiLevel);

    // Set/get GLES major/minor version.
    void setGlesVersion(int maj, int min);
    void getGlesVersion(int* maj, int* min);

    // Set/get renderer
    void setRenderer(SelectedRenderer renderer);
    SelectedRenderer getRenderer();

    // using Profiler = android::base::Profiler;
}
