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

#include "android/utils/compiler.h"
#include "android/utils/debug.h"

ANDROID_BEGIN_HEADER

#define GLES12TR_DLOG 0

#if GLES12TR_DLOG
#define APITRACE() do { \
    VERBOSE_TID_FUNCTION_DPRINT(gles1emu, "(gles1->2 translator) "); \
} while (0)
#define DLOG(...) do { \
    VERBOSE_TID_FUNCTION_DPRINT(gles1emu, "(gles1->2 translator): ", ##__VA_ARGS__); \
} while (0)
#else
#define APITRACE()
#define DLOG(...)
#endif

ANDROID_END_HEADER
