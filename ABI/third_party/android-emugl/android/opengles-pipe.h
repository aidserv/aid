/*
 * Copyright (C) 2011 The Android Open Source Project
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

#pragma once

#include "android/utils/compiler.h"
#include "android/utils/looper.h"

ANDROID_BEGIN_HEADER

// Initialize the 'opengles' pipe - the one used for GPU emulation protocol
// between guest and the emugl library.
// |dummyLooper| is now unused, this will be removed in a future patch.
void android_init_opengles_pipe(void);
// add
void android_init_opengles_tcp(void);

ANDROID_END_HEADER
