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

#include "android/utils/file_data.h"

#include <gtest/gtest.h>

class ScopedFileData {
public:
    ScopedFileData() : mStatus(0) { fileData_initEmpty(&mFileData); }

    ScopedFileData(const void* buff, size_t length) {
        mStatus = fileData_initFromMemory(&mFileData, buff, length);
    }

    explicit ScopedFileData(const ScopedFileData& other) {
        mStatus = fileData_initFrom(&mFileData, other.ptr());
    }

    explicit ScopedFileData(const FileData* other) {
        mStatus = fileData_initFrom(&mFileData, other);
    }

    ~ScopedFileData() { fileData_done(&mFileData); }

    int status() const { return mStatus; }
    FileData* ptr() { return &mFileData; }
    const FileData* ptr() const { return &mFileData; }
    FileData& operator*() { return mFileData; }
    FileData* operator->() { return &mFileData; }
private:
    FileData mFileData;
    int mStatus;
};

TEST(FileData, IsValid) {
    EXPECT_FALSE(fileData_isValid(NULL));

    FileData fakeData = { (uint8_t*)0x012345678, 12345, 23983 };
    EXPECT_FALSE(fileData_isValid(&fakeData));
}

TEST(FileData, InitializerConstant) {
    FileData data = FILE_DATA_INIT;
    EXPECT_TRUE(fileData_isValid(&data));
    EXPECT_TRUE(fileData_isEmpty(&data));
}

TEST(FileData, InitializerIsFullOfZeroes) {
    FileData data;
    memset(&data, 0, sizeof data);
    EXPECT_TRUE(fileData_isEmpty(&data));
    EXPECT_TRUE(fileData_isValid(&data));
}

TEST(FileData, EmptyInitializer) {
    ScopedFileData data;
    EXPECT_EQ(0, data.status());
    EXPECT_TRUE(fileData_isEmpty(data.ptr()));
}

TEST(FileData, InitEmpty) {
    ScopedFileData data("", 0U);
    EXPECT_EQ(0, data.status());
    EXPECT_TRUE(fileData_isEmpty(data.ptr()));
    EXPECT_FALSE(data->data);
    EXPECT_EQ(0U, data->size);
}

TEST(FileData, InitFromMemory) {
    static const char kData[] = "Hello World!";
    ScopedFileData data(kData, sizeof kData);
    EXPECT_EQ(0, data.status());
    EXPECT_NE(kData, (const char*)data->data);
    for (size_t n = 0; n < sizeof kData; ++n) {
        EXPECT_EQ(kData[n], data->data[n]) << "index " << n;
    }
}

TEST(FileData, InitFromOther) {
    static const char kData[] = "Hello World!";
    ScopedFileData data1(kData, sizeof kData);
    EXPECT_EQ(0, data1.status());

    ScopedFileData data2(data1.ptr());
    EXPECT_EQ(0, data2.status());

    EXPECT_EQ(data1->size, data2->size);
    EXPECT_NE(data1->data, data2->data);
    for (size_t n = 0; n < data1->size; ++n) {
        EXPECT_EQ(kData[n], data1->data[n]) << "index " << n;
        EXPECT_EQ(kData[n], data2->data[n]) << "index " << n;
    }
}

TEST(FileData, Swap) {
    static const char kData[] = "Hello World!";
    ScopedFileData data1(kData, sizeof kData);
    EXPECT_EQ(0, data1.status());

    ScopedFileData data2;
    EXPECT_EQ(0, data2.status());
    fileData_swap(data1.ptr(), data2.ptr());

    EXPECT_TRUE(fileData_isEmpty(data1.ptr()));
    EXPECT_FALSE(fileData_isEmpty(data2.ptr()));
    EXPECT_EQ(sizeof kData, data2->size);
    for (size_t n = 0; n < data2->size; ++n) {
        EXPECT_EQ(kData[n], data2->data[n]) << "index " << n;
    }
}
