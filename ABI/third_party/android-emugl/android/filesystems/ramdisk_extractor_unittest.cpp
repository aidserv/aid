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

#include "android/filesystems/ramdisk_extractor.h"

#include "android/base/EintrWrapper.h"
#include "android/filesystems/testing/TestSupport.h"

#include <gtest/gtest.h>

#include <stdio.h>

namespace {

#include "android/filesystems/testing/TestRamdiskImage.h"

class RamdiskExtractorTest : public ::testing::Test {
public:
    RamdiskExtractorTest() :
        mTempFilePath(android::testing::CreateTempFilePath()) {}

    bool fillData(const void* data, size_t dataSize) {
        FILE* file = ::fopen(mTempFilePath.c_str(), "wb");
        if (!file) {
            return false;
        }

        bool result = (fwrite(data, dataSize, 1, file) == 1);
        fclose(file);
        return result;
    }

    ~RamdiskExtractorTest() {
        if (!mTempFilePath.empty()) {
            HANDLE_EINTR(unlink(mTempFilePath.c_str()));
        }
    }

    const char* path() const { return mTempFilePath.c_str(); }

private:
    std::string mTempFilePath;
};

}  // namespace

TEST_F(RamdiskExtractorTest, FindFoo) {
    static const char kExpected[] = "Hello World!\n";
    static const size_t kExpectedSize = sizeof(kExpected) - 1U;
    char* out = NULL;
    size_t outSize = 0;

    EXPECT_TRUE(fillData(kTestRamdiskImage, kTestRamdiskImageSize));
    EXPECT_TRUE(android_extractRamdiskFile(path(), "foo", &out, &outSize));
    EXPECT_EQ(kExpectedSize, outSize);
    EXPECT_TRUE(out);
    EXPECT_TRUE(!memcmp(out, kExpected, outSize));
    free(out);
}

TEST_F(RamdiskExtractorTest, FindBar2) {
    static const char kExpected[] = "La vie est un long fleuve tranquille\n";
    static const size_t kExpectedSize = sizeof(kExpected) - 1U;
    char* out = NULL;
    size_t outSize = 0;

    EXPECT_TRUE(fillData(kTestRamdiskImage, kTestRamdiskImageSize));
    EXPECT_TRUE(android_extractRamdiskFile(path(), "bar2", &out, &outSize));
    EXPECT_EQ(kExpectedSize, outSize);
    EXPECT_TRUE(out);
    EXPECT_TRUE(!memcmp(out, kExpected, outSize));
    free(out);
}

TEST_F(RamdiskExtractorTest, FindZoo) {
    static const char kExpected[] = "Meow!!\n";
    static const size_t kExpectedSize = sizeof(kExpected) - 1U;
    char* out = NULL;
    size_t outSize = 0;

    EXPECT_TRUE(fillData(kTestRamdiskImage, kTestRamdiskImageSize));
    EXPECT_TRUE(android_extractRamdiskFile(path(), "zoo", &out, &outSize));
    EXPECT_EQ(kExpectedSize, outSize);
    EXPECT_TRUE(out);
    EXPECT_TRUE(!memcmp(out, kExpected, outSize));
    free(out);
}

TEST_F(RamdiskExtractorTest, MissingFile) {
    char* out = NULL;
    size_t outSize = 0;
    EXPECT_TRUE(fillData(kTestRamdiskImage, kTestRamdiskImageSize));
    EXPECT_FALSE(android_extractRamdiskFile(path(), "zoolander", &out, &outSize));
}
