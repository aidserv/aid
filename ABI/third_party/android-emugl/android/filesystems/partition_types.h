// Copyright 2014 The Android Open Source Project
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

#include "android/utils/compiler.h"

#include <inttypes.h>

ANDROID_BEGIN_HEADER

// List of supported Android partition image types.
typedef enum {
    ANDROID_PARTITION_TYPE_UNKNOWN = 0,
    ANDROID_PARTITION_TYPE_YAFFS2 = 1,
    ANDROID_PARTITION_TYPE_EXT4 = 2,
} AndroidPartitionType;

// Return a string describing the partition type to the caller.
// Note: this will panic if |part_type| is an invalid value.
const char* androidPartitionType_toString(AndroidPartitionType part_type);

// Return an AndroidPartitionType from a string description.
AndroidPartitionType androidPartitionType_fromString(const char* part_type);

// Probe a given image file and return its partition image type.
// Note: this returns ANDROID_PARTITION_TYPE_UNKNOWN if the file does
// not exist or cannot be read.
AndroidPartitionType androidPartitionType_probeFile(const char* image_file);

// Create or reset the file at |image_file| to be an empty partition of type
// |part_type| and size |part_size|. Returns 0 on success, or -errno on
// failure.
int androidPartitionType_makeEmptyFile(AndroidPartitionType part_type,
                                       uint64_t part_size,
                                       const char* image_file);

ANDROID_END_HEADER
