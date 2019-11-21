/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef GRAPHICS_TRANSLATION_UNDERLYING_APIS_H_
#define GRAPHICS_TRANSLATION_UNDERLYING_APIS_H_

#include "angle_gles2.h"

// Angle Underlying APIS
struct UnderlyingApis {
    ANGLE_GLES2* angle;
    // other entries here could apply to specific extensions, like
    //    const ANGLE_GLES2_ext *angle_ext
    // assuming that is how UndelryingApis implement them
};
typedef UnderlyingApis GraphicsApis;

// Swiftshader could offer different UnderlyingApis

#endif  // GRAPHICS_TRANSLATION_UNDERLYING_APIS_H_
