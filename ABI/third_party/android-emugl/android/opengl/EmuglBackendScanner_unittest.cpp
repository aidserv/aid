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

#include "android/opengl/EmuglBackendScanner.h"

#include "android/base/system/System.h"
#include "android/base/testing/TestSystem.h"
#include "android/base/testing/TestTempDir.h"
#include "android/utils/path.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include <fcntl.h>
#include <unistd.h>

using android::base::System;
using android::base::TestSystem;
using android::base::TestTempDir;

static void make_dir(const std::string& path) {
#ifdef _WIN32
    EXPECT_EQ(0, ::mkdir(path.c_str()));
#else
    EXPECT_EQ(0, ::mkdir(path.c_str(), 0755));
#endif
}

static void make_subdir(const std::string& path, const char* subdir) {
    std::string dir = path;
    dir.append("/");
    dir.append(subdir);
    make_dir(dir);
}

static void make_subfile(const std::string& dir, const char* file) {
    std::string path = dir;
    path.append("/");
    path.append(file);
    int fd = ::open(path.c_str(), O_WRONLY|O_CREAT, 0755);
    EXPECT_GE(fd, 0);
    ::close(fd);
}

namespace android {
namespace opengl {

TEST(EmuglBackendScanner, noLibDir) {
    TestTempDir myDir("emugl_backend_scanner");
    // Don't create any files
    std::vector<std::string> names = EmuglBackendScanner::scanDir(myDir.path());
    EXPECT_TRUE(names.empty());
}

TEST(EmuglBackendScanner, noBackends) {
    TestTempDir myDir("emugl_backend_scanner");
    // Create lib directory.
    std::string libDir(myDir.path());
    libDir += "/";
    libDir += System::kLibSubDir;
    make_dir(libDir);

    // Don't create any files
    std::vector<std::string> names = EmuglBackendScanner::scanDir(myDir.path());
    EXPECT_TRUE(names.empty());
}

TEST(EmuglBackendScanner, listBackends) {
    TestSystem testSys("foo", System::kProgramBitness);
    TestTempDir* myDir = testSys.getTempRoot();

    // Create lib directory.
    std::string libDir(myDir->path());
    libDir += "/foo/";
    make_dir(libDir);
    libDir += System::kLibSubDir;
    make_dir(libDir);

    // Create first backend sub-directory.

    // to be ignored, doesn't begin with 'gles_'
    make_subdir(libDir, "first");

    // second entry for the backend named 'second'
    make_subdir(libDir, "gles_second");

    // should be ignored, must have something after 'gles_' prefix.
    make_subdir(libDir, "gles_");

    // should be ignored: is a file, not a directory.
    make_subfile(libDir, "gles_fourth");

    // should be the first returned backend, due to alphabetical order.
    make_subdir(libDir, "gles_fifth");

    // should be returned as the third backend, due to alphabetical order.
    make_subdir(libDir, "gles_sixth");

    // Now check the scanner
    std::vector<std::string> names = EmuglBackendScanner::scanDir(PATH_SEP "foo");
    ASSERT_EQ(3U, names.size());
    EXPECT_STREQ("fifth", names[0].c_str());
    EXPECT_STREQ("second", names[1].c_str());
    EXPECT_STREQ("sixth", names[2].c_str());
}

TEST(EmuglBackendScanner, listBackendsWithProgramBitness) {
    TestSystem testSys("foo", 32);
    TestTempDir* myDir = testSys.getTempRoot();

    myDir->makeSubDir("foo");
    myDir->makeSubDir("foo/lib");
    myDir->makeSubDir("foo/lib/gles_first");
    myDir->makeSubDir("foo/lib/gles_second");
    myDir->makeSubDir("foo/lib/gles_third");

    myDir->makeSubDir("foo/lib64");
    myDir->makeSubDir("foo/lib64/gles_fourth");
    myDir->makeSubDir("foo/lib64/gles_fifth");
    myDir->makeSubDir("foo/lib64/gles_sixth");

    std::vector<std::string> names = EmuglBackendScanner::scanDir(PATH_SEP "foo", 32);
    ASSERT_EQ(3U, names.size());
    EXPECT_STREQ("first", names[0].c_str());
    EXPECT_STREQ("second", names[1].c_str());
    EXPECT_STREQ("third", names[2].c_str());

    names = EmuglBackendScanner::scanDir(PATH_SEP "foo", 64);
    ASSERT_EQ(3U, names.size());
    EXPECT_STREQ("fifth", names[0].c_str());
    EXPECT_STREQ("fourth", names[1].c_str());
    EXPECT_STREQ("sixth", names[2].c_str());
}

}  // namespace opengl
}  // namespace android
