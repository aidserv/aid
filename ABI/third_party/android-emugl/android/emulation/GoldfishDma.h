/* Copyright (C) 2016 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

#pragma once

#include "android/base/files/Stream.h"

#include <inttypes.h>

static const uint32_t kDmaBufSizeMB = 32;
// GOLDFISH DMA
//
// Goldfish DMA is an extension to the pipe device
// and is designed to facilitate high-speed RAM->RAM
// transfers from guest to host.
//
// See docs/GOLDFISH-VIRTUAL-HARDWARE.txt and
//     docs/ANDROID-QEMU-PIPE.txt
//
// for more details.
// Host / virtual device interface:
typedef struct {
// add_buffer():
// Tell us that there is a physically-contiguous buffer in the guest
// starting at |guest_paddr|. This should be used upon allocation
// of the DMA buffer in the ugest.
void (*add_buffer)(void* pipe, uint64_t guest_paddr, uint64_t sz);
// remove_buffer():
// Tell us that we don't care about tracking this buffer anymore.
// This should usually be used when the DMA buffer has been freed
// in the ugest.
void (*remove_buffer)(uint64_t guest_paddr);
// get_host_addr():
// Obtain a pointer to guest physical memory that is usable as
// as host void*.
void* (*get_host_addr)(uint64_t guest_paddr);
// invalidate_host_mappings():
// Sometimes (say, on snapshot save/load) we may need to re-map
// the guest DMA buffers so to regain access to them.
// This function tells our map of DMA buffers to remap the buffers
// next time they are used.
void (*invalidate_host_mappings)(void);
// unlock():
// Unlocks the buffer at |guest_paddr| to signal the guest
// that we are done writing it.
void (*unlock)(uint64_t guest_paddr);
// reset_host_mappings();
// Not only invalidates the mappings, but also removes them from the record.
void (*reset_host_mappings)(void);
// For snapshots.
void (*save_mappings)(android::base::Stream* stream);
void (*load_mappings)(android::base::Stream* stream);
} GoldfishDmaOps;

extern GoldfishDmaOps android_goldfish_dma_ops;
