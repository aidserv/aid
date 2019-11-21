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

#include "OpenglRender/render_api_types.h"
#include "emugl/common/sync_device.h"
#if 0
typedef void (*emugl_logger_t)(const char* fmt, ...);
typedef struct {
    emugl_logger_t coarse;
    emugl_logger_t fine;
} emugl_logger_struct;
#endif // add
extern emugl_logger_t emugl_logger;
extern emugl_logger_t emugl_cxt_logger;
void set_emugl_logger(emugl_logger_t f);
void set_emugl_cxt_logger(emugl_logger_t f);

#define GL_LOGGING 1

#if GL_LOGGING

#define GL_LOG(...) do { \
    emugl_logger(__VA_ARGS__); \
} while (0)

#define GL_CXT_LOG(...) do { \
    emugl_cxt_logger(__VA_ARGS__); \
} while (0)

#else
#define GL_LOG(...) 0
#endif
