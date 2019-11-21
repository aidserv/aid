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
#include "emugl/common/sync_device.h"

extern emugl_dma_add_buffer_t g_emugl_dma_add_buffer;
extern emugl_dma_remove_buffer_t g_emugl_dma_remove_buffer;
extern emugl_dma_get_host_addr_t g_emugl_dma_get_host_addr;
extern emugl_dma_unlock_t g_emugl_dma_unlock;

void set_emugl_dma_add_buffer(emugl_dma_add_buffer_t);
void set_emugl_dma_remove_buffer(emugl_dma_remove_buffer_t);
void set_emugl_dma_get_host_addr(emugl_dma_get_host_addr_t);
void set_emugl_dma_invalidate_host_mappings(emugl_dma_invalidate_host_mappings_t);
void set_emugl_dma_unlock(emugl_dma_unlock_t);
