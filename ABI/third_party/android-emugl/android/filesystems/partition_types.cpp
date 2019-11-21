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

#include "android/filesystems/partition_types.h"

#include "android/filesystems/ext4_utils.h"
#include "android/utils/panic.h"
#include "android/utils/path.h"

#include <errno.h>

namespace {

const struct {
    const char* name;
    AndroidPartitionType value;
} kPartitionTypeMap[] = {
    { "unknown", ANDROID_PARTITION_TYPE_UNKNOWN },
    { "yaffs2", ANDROID_PARTITION_TYPE_YAFFS2 },
    { "ext4", ANDROID_PARTITION_TYPE_EXT4 },
};

const size_t kPartitionTypeMapSize =
        sizeof(kPartitionTypeMap) / sizeof(kPartitionTypeMap[0]);

}  // namespace

const char* androidPartitionType_toString(AndroidPartitionType part_type) {
    for (size_t n = 0; n < kPartitionTypeMapSize; ++n) {
        if (kPartitionTypeMap[n].value == part_type) {
            return kPartitionTypeMap[n].name;
        }
    }
    APANIC("Invalid partition type value %d", part_type);
    return "unknown";
}

AndroidPartitionType androidPartitionType_fromString(const char* part_type) {
    for (size_t n = 0; n < kPartitionTypeMapSize; ++n) {
        if (!strcmp(kPartitionTypeMap[n].name, part_type)) {
            return kPartitionTypeMap[n].value;
        }
    }
    return ANDROID_PARTITION_TYPE_UNKNOWN;
}

AndroidPartitionType androidPartitionType_probeFile(const char* image_file) {
    if (!path_exists(image_file)) {
        return ANDROID_PARTITION_TYPE_UNKNOWN;
    }
    if (android_pathIsExt4PartitionImage(image_file)) {
        return ANDROID_PARTITION_TYPE_EXT4;
    }
    // Assume YAFFS2, since there is little way to be sure for now.
    // NOTE: An empty file is a valid Yaffs2 file!
    return ANDROID_PARTITION_TYPE_YAFFS2;
}


int androidPartitionType_makeEmptyFile(AndroidPartitionType part_type,
                                       uint64_t part_size,
                                       const char* part_file) {
    switch (part_type) {
        case ANDROID_PARTITION_TYPE_YAFFS2:
            // Any empty file is a valid YAFFS2 partition, |part_size|
            // can be ignored here.
            if (path_empty_file(part_file) < 0) {
                return -errno;
            }
            return 0;

        case ANDROID_PARTITION_TYPE_EXT4:
            return android_createEmptyExt4Image(part_file, part_size, NULL);

        default:
            return -EINVAL;
    }
}
