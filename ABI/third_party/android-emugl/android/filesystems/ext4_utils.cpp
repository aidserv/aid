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

#include "android/filesystems/ext4_utils.h"

#include "android/base/Log.h"
#include "android/utils/debug.h"
#include "android/base/files/ScopedStdioFile.h"

#include "make_ext4fs.h"

#include <memory>
#include <stdint.h>
#include <string.h>

#define DEBUG_EXT4  0

#define EXT4_LOG     LOG_IF(INFO, DEBUG_EXT4)
#define EXT4_PLOG    PLOG_IF(INFO, DEBUG_EXT4)
#define EXT4_ERROR   LOG_IF(ERROR, DEBUG_EXT4)
#define EXT4_PERROR  PLOG_IF(ERROR, DEBUG_EXT4)

struct Ext4Magic {
    static const size_t kOffset = 0x438U;
    static const size_t kSize = 2U;
    static const uint8_t kExpected[kSize];
};

const uint8_t Ext4Magic::kExpected[kSize] = { 0x53, 0xef };

bool android_pathIsExt4PartitionImage(const char* path) {
    if (!path) {
        EXT4_ERROR << "NULL path parameter";
        return false;
    }

    android::base::ScopedStdioFile file(::fopen(path, "rb"));
    if (!file.get()) {
        EXT4_PERROR << "Could not open file: " << path;
        return false;
    }

    if (::fseek(file.get(), Ext4Magic::kOffset, SEEK_SET) != 0) {
        EXT4_LOG << "Can't seek to byte " << Ext4Magic::kOffset
                 << " of " << path;
        return false;
    }

    char magic[Ext4Magic::kSize];
    if (::fread(magic, sizeof(magic), 1, file.get()) != 1) {
        EXT4_PLOG << "Could not read " << sizeof(magic)
                  << " bytes from " << path;
        return false;
    }

    if (!::memcmp(magic, Ext4Magic::kExpected, sizeof(magic))) {
        EXT4_LOG << "File is Ext4 partition image: " << path;
        return true;
    }

    EXT4_LOG << "Not an Ext4 partition image: " << path;
    return false;
}

int android_createEmptyExt4Image(const char *filePath,
                                 uint64_t size,
                                 const char *mountpoint) {
    return android_createExt4ImageFromDir(filePath, NULL, size, mountpoint);
}

int android_createExt4ImageFromDir(const char *dstFilePath,
                                   const char *srcDirectory,
                                   uint64_t size,
                                   const char *mountpoint) {

    int ret = ::make_ext4fs_from_dir(dstFilePath, srcDirectory, size,
                                     mountpoint, NULL, android_verbose ? 1 : -1);
    if (ret < 0)
        EXT4_ERROR << "Failed to create ext4 image at: " << dstFilePath;
    return ret;
}
