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

// #include "OpenglRender/render_api_types.h"
#if 1

#include "android/featurecontrol/Features.h"
typedef unsigned long long uint64_t;
typedef unsigned int uint32_t;

typedef uint64_t (*emugl_sync_create_timeline_t)();
typedef int (*emugl_sync_create_fence_t)(uint64_t timeline, uint32_t pt);
typedef void (*emugl_sync_timeline_inc_t)(uint64_t timeline, uint32_t howmuch);
typedef void (*emugl_sync_destroy_timeline_t)(uint64_t timeline);

typedef void (*emugl_sync_trigger_wait_t)(uint64_t glsync, uint64_t thread, uint64_t timeline);
typedef void (*emugl_sync_register_trigger_wait_t)(emugl_sync_trigger_wait_t trigger_fn);

typedef bool (*emugl_sync_device_exists_t)();

typedef void (*emugl_logger_t)(const char* fmt, ...);
typedef struct {
    emugl_logger_t coarse;
    emugl_logger_t fine;
} emugl_logger_struct;

typedef bool (*emugl_feature_is_enabled_t)(android::featurecontrol::Feature);

typedef void (*emugl_dma_add_buffer_t)(void*, uint64_t, uint64_t);
typedef void (*emugl_dma_remove_buffer_t)(uint64_t);
typedef void* (*emugl_dma_get_host_addr_t)(uint64_t);
typedef void (*emugl_dma_invalidate_host_mappings_t)(void);
typedef void (*emugl_dma_unlock_t)(uint64_t);

typedef struct {
    emugl_dma_add_buffer_t add_buffer;
    emugl_dma_remove_buffer_t remove_buffer;
    emugl_dma_get_host_addr_t get_host_addr;
    emugl_dma_invalidate_host_mappings_t invalidate_host_mappings;
    emugl_dma_unlock_t unlock;
} emugl_dma_ops;
#endif //add

extern emugl_sync_create_timeline_t emugl_sync_create_timeline;
extern emugl_sync_create_fence_t emugl_sync_create_fence;
extern emugl_sync_timeline_inc_t emugl_sync_timeline_inc;
extern emugl_sync_destroy_timeline_t emugl_sync_destroy_timeline;
extern emugl_sync_register_trigger_wait_t emugl_sync_register_trigger_wait;
extern emugl_sync_device_exists_t emugl_sync_device_exists;

void set_emugl_sync_create_timeline(emugl_sync_create_timeline_t);
void set_emugl_sync_create_fence(emugl_sync_create_fence_t);
void set_emugl_sync_timeline_inc(emugl_sync_timeline_inc_t);
void set_emugl_sync_destroy_timeline(emugl_sync_destroy_timeline_t);
void set_emugl_sync_register_trigger_wait(emugl_sync_register_trigger_wait_t trigger_fn);
void set_emugl_sync_device_exists(emugl_sync_device_exists_t);
