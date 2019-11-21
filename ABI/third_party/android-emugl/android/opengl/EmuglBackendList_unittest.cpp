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

#include "android/opengl/EmuglBackendList.h"

#include "android/base/StringFormat.h"
#include "android/base/testing/TestSystem.h"
#include "android/base/testing/TestTempDir.h"

#include <gtest/gtest.h>

namespace android {
namespace opengl {

#define ARRAYLEN(x)  (sizeof(x)/sizeof(x[0]))

using android::base::StringFormat;
using android::base::System;
using android::base::TestTempDir;
using android::base::TestSystem;

static std::string makeLibSubPath(const char* name) {
    return StringFormat("foo/%s/%s", System::kLibSubDir, name);
}

static void makeLibSubDir(TestTempDir* dir, const char* name) {
    dir->makeSubDir(makeLibSubPath(name).c_str());
}

static void makeLibSubFile(TestTempDir* dir, const char* name) {
    dir->makeSubFile(makeLibSubPath(name).c_str());
}

TEST(EmuglBackendList, init) {
    TestSystem testSys("foo", System::kProgramBitness, "/");
    TestTempDir* myDir = testSys.getTempRoot();

    myDir->makeSubDir("foo");

    makeLibSubDir(myDir, "");
    makeLibSubDir(myDir, "gles_first");
    makeLibSubFile(myDir, "gles_second");  // should be ignored (file).
    makeLibSubDir(myDir, "gles_");  // should be ignored (no name).
    makeLibSubDir(myDir, "gles_fourth");
    makeLibSubDir(myDir, "gles_fifth");

    EmuglBackendList list("foo", System::kProgramBitness);

    // NOTE: Must appear in alphabetical order
    const char* const kExpected[] = {
        "fifth", "first", "fourth",
    };
    const size_t kExpectedLen = ARRAYLEN(kExpected);

    const std::vector<std::string>& names = list.names();
    EXPECT_EQ(kExpectedLen, names.size());
    for (size_t n = 0; n < kExpectedLen; ++n) {
        EXPECT_STREQ(kExpected[n], names[n].c_str()) << "#" << n;
        EXPECT_TRUE(list.contains(kExpected[n]));
    }
}

TEST(EmuglBackendList, getBackendLibPath) {
    TestSystem testSys("foo", System::kProgramBitness, "/");
    TestTempDir* myDir = testSys.getTempRoot();

    myDir->makeSubDir("foo");

    makeLibSubDir(myDir, "");
    makeLibSubDir(myDir, "gles_bar");

    static const struct {
        EmuglBackendList::Library library;
        const char* libName;
    } kData[] = {
#ifdef _WIN32
        { EmuglBackendList::LIBRARY_EGL, "libEGL.dll" },
        { EmuglBackendList::LIBRARY_GLESv1, "libGLES_CM.dll" },
        { EmuglBackendList::LIBRARY_GLESv2, "libGLESv2.dll" },
#elif defined(__APPLE__)
        { EmuglBackendList::LIBRARY_EGL, "libEGL.dylib" },
        { EmuglBackendList::LIBRARY_GLESv1, "libGLES_CM.dylib" },
        { EmuglBackendList::LIBRARY_GLESv2, "libGLESv2.dylib" },
#else
        { EmuglBackendList::LIBRARY_EGL, "libEGL.so" },
        { EmuglBackendList::LIBRARY_GLESv1, "libGLES_CM.so" },
        { EmuglBackendList::LIBRARY_GLESv2, "libGLESv2.so" },
#endif
    };
    const size_t kDataLen = ARRAYLEN(kData);

    for (size_t n = 0; n < kDataLen; ++n) {
        std::string file = "gles_bar/";
        file += kData[n].libName;
        makeLibSubFile(myDir, file.c_str());
    }

    EmuglBackendList list("/foo", System::kProgramBitness);
    const std::vector<std::string>& names = list.names();

    EXPECT_EQ(1U, names.size());
    EXPECT_STREQ("bar", names[0].c_str());

    for (size_t n = 0; n < kDataLen; ++n) {
        std::string expected = StringFormat("/foo/%s/gles_bar/%s",
                                            System::kLibSubDir,
                                            kData[n].libName);
        std::string libdir;
        EXPECT_TRUE(list.getBackendLibPath("bar", kData[n].library, &libdir));
        EXPECT_TRUE(list.contains("bar"));
        EXPECT_FALSE(list.contains("foo"));
        EXPECT_STREQ(expected.c_str(), libdir.c_str());
    }
}

TEST(EmuglBackend, defaultName) {
    EmuglBackendList list("foo", 0);
    EXPECT_STREQ("auto", list.defaultName().c_str());
}

}  // namespace opengl
}  // namespace android


