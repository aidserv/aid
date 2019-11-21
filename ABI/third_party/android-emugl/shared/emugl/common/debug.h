/*
* Copyright (C) 2016 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#pragma once

// Usage: Define EMUGL_DEBUG_LEVEL before including this header to
//        select the behaviour of the D() and DD() macros.

#if defined(EMUGL_DEBUG_LEVEL) && EMUGL_DEBUG_LEVEL > 0
#include <stdio.h>
#define D(...)  (printf("%s:%d:%s: ", __FILE__, __LINE__, __func__), printf(__VA_ARGS__), printf("\n"), fflush(stdout))
#else
#define D(...)  (void)0
#endif // EMUGL_DEBUG_LEVEL > 0

#if defined(EMUGL_DEBUG_LEVEL) && EMUGL_DEBUG_LEVEL > 1
#define DD(...) D(__VA_ARGS__)
#else
#define DD(...) (void)0
#endif // EMUGL_DEBUG_LEVEL > 1
