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

#include <string>

namespace android {
namespace internal {

// Testing the logic of the partition configuration setup code for all
// different cases is complex. This class is an abstract interface used
// to simplify unit-testing. Its methods are called from
// android_partition_configuration_setup() to access the host system.
// The default implementation can be obtained with
// PartitionConfigBackend::get(), though a unit-test would provide its
// own sub-class, and use
// android_partition_configuration_set_backend_for_testing()
// to inject it.
class PartitionConfigBackend {
public:
    // Constructor.
    PartitionConfigBackend() {}

    // Destructor.
    virtual ~PartitionConfigBackend() {}

    // Retrieve current process-wide instance.
    static PartitionConfigBackend* get();

    // Change process-wide instance to new one during unit-testing.
    // Return previous instance value, to be restored when the test completes.
    static PartitionConfigBackend* setForTesting(
            PartitionConfigBackend* newBackend);

    // Return true iff host |path| exists.
    virtual bool pathExists(const char* path) = 0;

    // Create an empty file at a given location. If the file already
    // exists, it is truncated without warning. Return true on success,
    // false/errno on failure.
    virtual bool pathEmptyFile(const char* path) = 0;

    // Copy host file |sourcePath| into |destPath|. Return true on success,
    // false/errno on failure.
    virtual bool pathCopyFile(const char* dest, const char* source) = 0;

    // Try to lock host file at |path|. Return true on success, false
    // on failure.
    virtual bool pathLockFile(const char* path) = 0;

    // Create an empty temporary file. On success return true and
    // sets |*path| to its path. On error return false/errno.
    virtual bool pathCreateTempFile(std::string* path) = 0;

    // Probe the file at |path| and return the equivalent partition type.
    // On failure, return ANDROID_PARTITION_TYPE_UNKNOWN.
    virtual AndroidPartitionType probePartitionFileType(const char* path) = 0;

    // Extract the content of a file from a ramdisk.img partition.
    // |ramdisk_path| is the image's host file path.
    // |file_path| is the file's path within the ramdisk image.
    // On success, return true and sets |*out| to the file's content.
    virtual bool extractRamdiskFile(const char* ramdisk_path,
                                    const char* file_path,
                                    std::string* out) = 0;

    // Parse an 'fstab' file and extract the type of a given partition.
    // |fstab| points to the content of the fstab file in memory, |mountPath|
    // is the partition's mount point as it appears in the fstab. On success,
    // return true and sets |*partitionFormat| to the corresponding partition
    // format (e.g. 'ext4'). On failure, return false.
    virtual bool parsePartitionFormat(const std::string& fstab,
                                      const char* mountPath,
                                      std::string* partitionFormat) = 0;

    // Create an empty partition file on the host.
    // |partitionType| is the partition format, |partitionSize| is the
    // size of the virtual partition (the actual file can be smaller),
    // and |partitionPath| is the host path of the partition file.
    // Return true on success, false/errno on failure.
    virtual bool makeEmptyPartition(AndroidPartitionType partitionType,
                                    uint64_t partitionSize,
                                    const char* partitionPath) = 0;

    // Resize an existing ext4 partition at |partitionPath| to a new
    // size in bytes given by |partitionSize|.
    virtual void resizeExt4Partition(const char* partitionPath,
                                     uint64_t partitionSize) = 0;

private:
    static PartitionConfigBackend* sInstance;
};

}  // namespace internal
}  // namespace android
