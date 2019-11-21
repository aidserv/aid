// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include "android/filesystems/partition_types.h"
#include "android/utils/compiler.h"

#include <stdbool.h>
#include <stdint.h>

ANDROID_BEGIN_HEADER

// Pointer to a function used to setup an emulated NAND partition at runtime.
// This is called from within android_partition_config() below.
// |opaque| is the |setup_opaque| parameter from android_partition_config().
// |name| is the partition's name (e.g. 'system', 'userdata' or 'cache')
// |size| is its size in bytes.
// |path| is the file path for the partition image.
// |format| is the partition's type.
// |readonly| is true to indicate that the image is read-only.
typedef void (*AndroidPartitionSetupFunction)(void* opaque,
                                              const char* name,
                                              uint64_t size,
                                              const char* path,
                                              AndroidPartitionType format,
                                              bool readonly);

// A structure used to model the information related to a given partition
// image file that will have to be mounted at runtime as a virtual NAND
// device (QEMU1) or a virtio-block one (QEMU2).
// |size| is the partition size in bytes.
// |path| is the path to the file backing the partition image data.
// |init_path| can be NULL, or the path to the initialization content for
// the partition image. In other words, its content will be copied into |path|
// at emulator startup.
typedef struct {
    uint64_t size;
    const char* path;
    const char* init_path;
} AndroidPartitionInfo;

// A structure used to model the information related to all partition image
// files that will have to be mounted at runtime.
// |ramdisk_path| is the path to the AVD's ramdisk.img file, which is used to
// parse its 'fstab' file and determine each partition exact type.
// |fstab_name| is the name of the 'fstab' path in the ramdisk, which
// is board-dependent (e.g. 'fstab.goldfish' for QEMU1).
// |system_partition|, |userdata_partition| and |cache_partition| are
// descriptors of each of the corresponding partition.
// |kernel_supports_yaffs2| is true to indicate that the kernel supports the
// YAFFS2 file system.
// |wipe_data| can be true to indicate that the userdata partition needs to
// be wiped.
// |writable_system| can be true to indicate that a writable system partition
// is desired. This may create a temporary copy of the system partition image.
typedef struct {
    const char* ramdisk_path;
    const char* fstab_name;
    AndroidPartitionInfo system_partition;
    AndroidPartitionInfo data_partition;
    AndroidPartitionInfo cache_partition;
    bool kernel_supports_yaffs2;
    bool wipe_data;
    bool writable_system;
} AndroidPartitionConfiguration;

// Setup emulated NAND partition according to misc configuration parameters:
// |configuration| is an AndroidPartitionConfiguration instance describing
// the emulated NAND partitions.
// |setup_func| is a user-provided callback that will be called with the
// information relevant to initialize each virtual NAND disk / virtio block.
// |setup_opaque| is a user-provided value passed as the 1st parameter in
// |setup_func| calls.
// On success, return true. On failure, return false and sets |*error_message|
// to a human-friendly user message explaining the error. free() must be called
// by the user to release it.
bool android_partition_configuration_setup(
        const AndroidPartitionConfiguration* config,
        AndroidPartitionSetupFunction setup_func,
        void* setup_opaque,
        char** error_message);

ANDROID_END_HEADER
