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

#include "android/base/EintrWrapper.h"
#include "android/base/files/ScopedStdioFile.h"
#include "android/filesystems/ext4_utils.h"
#include "android/filesystems/testing/TestExt4ImageHeader.h"
#include "android/filesystems/testing/TestSupport.h"

#include "android/utils/path.h"

#include <gtest/gtest.h>

#include <stdio.h>
#include <string>
#include <unistd.h>

namespace {

using android::base::ScopedStdioFile;

class TempPartition {
public:
    TempPartition() {
        mPath = android::testing::CreateTempFilePath();
    }

    ~TempPartition() {
        if (!mPath.empty()) {
            HANDLE_EINTR(::unlink(mPath.c_str()));
        }
    }

    const char* GetPath() const {
        return mPath.c_str();
    }

protected:
    std::string mPath;
};

}  // namespace

TEST(AndroidPartitionType, ToString) {
    EXPECT_STREQ(
            "unknown",
            androidPartitionType_toString(ANDROID_PARTITION_TYPE_UNKNOWN));
    EXPECT_STREQ(
            "yaffs2",
            androidPartitionType_toString(ANDROID_PARTITION_TYPE_YAFFS2));
    EXPECT_STREQ(
            "ext4",
            androidPartitionType_toString(ANDROID_PARTITION_TYPE_EXT4));
}

TEST(AndroidPartitionType, FromString) {
    EXPECT_EQ(ANDROID_PARTITION_TYPE_YAFFS2,
              androidPartitionType_fromString("yaffs2"));
    EXPECT_EQ(ANDROID_PARTITION_TYPE_EXT4,
              androidPartitionType_fromString("ext4"));
    EXPECT_EQ(ANDROID_PARTITION_TYPE_UNKNOWN,
              androidPartitionType_fromString("unknown"));
    EXPECT_EQ(ANDROID_PARTITION_TYPE_UNKNOWN,
              androidPartitionType_fromString("foobar"));
}

TEST(AndroidPartitionType, ProbeFileYaffs2) {
    TempPartition part;

    // An empty partition is a valid YAFFS2 one.
    ::path_empty_file(part.GetPath());

    EXPECT_EQ(ANDROID_PARTITION_TYPE_YAFFS2,
              androidPartitionType_probeFile(part.GetPath()));
}


TEST(AndroidPartitionType, ProbeFileExt4) {
    TempPartition part;

    android_createEmptyExt4Image(part.GetPath(), 16*1024*1024, "cache");

    EXPECT_EQ(ANDROID_PARTITION_TYPE_EXT4,
              androidPartitionType_probeFile(part.GetPath()));
}

TEST(AndroidPartitionType, MakeEmptyFileYaffs2) {
    TempPartition part;

    EXPECT_EQ(0, androidPartitionType_makeEmptyFile(
            ANDROID_PARTITION_TYPE_YAFFS2,
            8 * 1024 * 1024,
            part.GetPath())) << "Could not create Yaffs2 partition image";

    EXPECT_EQ(ANDROID_PARTITION_TYPE_YAFFS2,
              androidPartitionType_probeFile(part.GetPath()));
}

TEST(AndroidPartitionType, MakeEmptyFileExt4) {
    TempPartition part;

    EXPECT_EQ(0, androidPartitionType_makeEmptyFile(
            ANDROID_PARTITION_TYPE_EXT4,
            8 * 1024 * 1024,
            part.GetPath())) << "Could not create EXT4 partition image";

    EXPECT_EQ(ANDROID_PARTITION_TYPE_EXT4,
              androidPartitionType_probeFile(part.GetPath()));
}
