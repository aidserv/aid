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

#include "android/utils/dirscanner.h"

#include "android/base/files/PathUtils.h"
#include "android/base/misc/StringUtils.h"
#include "android/base/testing/TestTempDir.h"

#include <gtest/gtest.h>

#include <algorithm>

#include <fcntl.h>
#include <unistd.h>

#define ARRAYLEN(x)  (sizeof(x)/sizeof(x[0]))

using android::base::TestTempDir;
using android::base::PathUtils;

static void make_subfile(const std::string& dir, const char* file) {
    std::string path = dir;
    path.append("/");
    path.append(file);
    int fd = ::open(path.c_str(), O_WRONLY|O_CREAT, 0755);
    EXPECT_GE(fd, 0);
    ::close(fd);
}

TEST(DirScanner, scanEmptyDir) {
    TestTempDir myDir("DirScannerTest");
    DirScanner* scanner = dirScanner_new(myDir.path());
    EXPECT_TRUE(scanner);
    EXPECT_EQ(0U, dirScanner_numEntries(scanner));
    EXPECT_FALSE(dirScanner_next(scanner));
    dirScanner_free(scanner);
}

TEST(DirScanner, scanNormal) {
    static const char* const kExpected[] = {
        "fifth", "first", "fourth", "second", "sixth", "third"
    };
    static const char* const kInput[] = {
        "first", "second", "third", "fourth", "fifth", "sixth"
    };
    const size_t kCount = ARRAYLEN(kInput);

    TestTempDir myDir("DirScannerTest");
    for (size_t n = 0; n < kCount; ++n) {
        make_subfile(myDir.path(), kInput[n]);
    }

    DirScanner* scanner = dirScanner_new(myDir.path());
    EXPECT_TRUE(scanner);
    EXPECT_EQ(kCount, dirScanner_numEntries(scanner));

    std::vector<std::string> entries;
    for (;;) {
        const char* entry = dirScanner_next(scanner);
        if (!entry) {
            break;
        }
        entries.push_back(std::string(entry));
    }
    dirScanner_free(scanner);

    // There is no guarantee on the order of files returned by
    // the file system, so sort them here to ensure consistent
    // comparisons.
    std::sort(entries.begin(), entries.end());

    EXPECT_EQ(kCount, entries.size());
    for (size_t n = 0; n < kCount; ++n) {
        EXPECT_STREQ(kExpected[n], entries[n].c_str()) << "#" << n;
    }
}

TEST(DirScanner, scanFull) {
    static const char* const kExpected[] = {
        "fifth", "first", "fourth", "second", "sixth", "third"
    };
    static const char* const kInput[] = {
        "first", "second", "third", "fourth", "fifth", "sixth"
    };
    const size_t kCount = ARRAYLEN(kInput);

    TestTempDir myDir("DirScannerTest");
    for (size_t n = 0; n < kCount; ++n) {
        make_subfile(myDir.path(), kInput[n]);
    }

    DirScanner* scanner = dirScanner_new(myDir.path());
    EXPECT_TRUE(scanner);
    EXPECT_EQ(kCount, dirScanner_numEntries(scanner));

    std::vector<std::string> entries;
    for (;;) {
        const char* entry = dirScanner_nextFull(scanner);
        if (!entry) {
            break;
        }
        entries.push_back(std::string(entry));
    }
    dirScanner_free(scanner);

    // There is no guarantee on the order of files returned by
    // the file system, so sort them here to ensure consistent
    // comparisons.
    std::sort(entries.begin(), entries.end());

    EXPECT_EQ(kCount, entries.size());
    for (size_t n = 0; n < kCount; ++n) {
        std::string expected =
                android::base::PathUtils::addTrailingDirSeparator(
                        std::string(myDir.path()));
        expected += kExpected[n];
        EXPECT_STREQ(expected.c_str(), entries[n].c_str()) << "#" << n;
    }
}
