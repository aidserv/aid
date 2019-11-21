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

#include "android/base/EintrWrapper.h"
#include "android/base/Log.h"
#include "android/base/files/ScopedStdioFile.h"
#include "android/filesystems/testing/TestExt4ImageHeader.h"
#include "android/filesystems/testing/TestSupport.h"

#include <gtest/gtest.h>

#include <string>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

using android::base::ScopedStdioFile;
using android::testing::CreateTempFilePath;

class Ext4UtilsTest : public ::testing::Test {
public:
    Ext4UtilsTest() : mTempFilePath() {
        ::memset(mImage, 0, sizeof mImage);
        ::memcpy(mImage, kTestExt4ImageHeader, kTestExt4ImageHeaderSize);
    }

    ~Ext4UtilsTest() {
        if (!mTempFilePath.empty()) {
            HANDLE_EINTR(unlink(mTempFilePath.c_str()));
        }
    }

protected:
    // Create a temporary file with up to |maxBytes| bytes of |mImage|
    // in it, and return its path. The file will be removed in the
    // destructor.
    const char* createTempFile(size_t maxBytes) {
        CHECK(maxBytes <= sizeof mImage);
        // TODO(digit): Replace with something else.
        mTempFilePath = CreateTempFilePath();
        ScopedStdioFile file(fopen(mTempFilePath.c_str(), "wb"));
        PCHECK(file.get()) << "Could not create temporary file!";
        PCHECK(::fwrite(mImage, maxBytes, 1, file.get()) == 1)
                << "Could not write " << maxBytes << " bytes to "
                << "temporary file";
        return mTempFilePath.c_str();
    }

    // Create the path of a temporary file, but do not create or
    // populate it. The file is removed in the destructor.
    const char* createTempPath() {
        mTempFilePath = CreateTempFilePath();
        return mTempFilePath.c_str();
    }

    std::string mTempFilePath;
    char mImage[kTestExt4ImageHeaderSize * 2U];
};

TEST_F(Ext4UtilsTest, android_pathIsExt4PartitionImage) {
    const char* path = createTempFile(kTestExt4ImageHeaderSize);
    EXPECT_TRUE(android_pathIsExt4PartitionImage(path));
}

TEST_F(Ext4UtilsTest, android_pathIsExt4PartitionImageTruncated) {
    const char* path = createTempFile(kTestExt4ImageHeaderSize - 2U);
    EXPECT_FALSE(android_pathIsExt4PartitionImage(path));
}

TEST_F(Ext4UtilsTest, android_pathIsExt4PartitionImageCorrupted) {
    memset(mImage, 0x55, sizeof mImage);
    const char* path = createTempFile(sizeof mImage);
    EXPECT_FALSE(android_pathIsExt4PartitionImage(path));
}

TEST_F(Ext4UtilsTest, android_pathIsExt4PartitionImageBadMagic1) {
    mImage[kTestExt4ImageHeaderSize - 1] = 0;
    const char* path = createTempFile(sizeof mImage);
    EXPECT_FALSE(android_pathIsExt4PartitionImage(path));
}

TEST_F(Ext4UtilsTest, android_pathIsExt4PartitionImageBadMagic2) {
    mImage[kTestExt4ImageHeaderSize - 2] = 0;
    const char* path = createTempFile(sizeof mImage);
    EXPECT_FALSE(android_pathIsExt4PartitionImage(path));
}

TEST_F(Ext4UtilsTest, android_createEmptyExt4Partition) {
    const char* tempPath = createTempPath();
    uint64_t kSize = 32 * 1024 * 1024;
    int ret = android_createEmptyExt4Image(tempPath, kSize, "cache");
    EXPECT_EQ(0, ret);
    EXPECT_TRUE(android_pathIsExt4PartitionImage(tempPath));
}
