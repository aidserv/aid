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

#include "android/qt/qt_path.h"

#include "android/base/testing/TestSystem.h"
#include "android/base/testing/TestTempDir.h"

#include <gtest/gtest.h>

#include <string>

using namespace android::base;

TEST(androidQtGetLibraryDir, Qt32) {
#if _WIN32
    const char basePath[]   = "\\foo";
    const char resultPath[] = "\\foo\\lib\\qt\\lib";
#else
    const char basePath[]  = "/foo";
    const char resultPath[] = "/foo/lib/qt/lib";
#endif

    TestSystem testSys(basePath, 32);

    EXPECT_STREQ(resultPath, androidQtGetLibraryDir(32).c_str());
}

TEST(androidQtGetLibraryDir, Qt64) {
#if _WIN32
    const char basePath[]   = "\\foo";
    const char resultPath[] = "\\foo\\lib64\\qt\\lib";
#else
    const char basePath[]  = "/foo";
    const char resultPath[] = "/foo/lib64/qt/lib";
#endif

    TestSystem testSys(basePath, 64);

    EXPECT_STREQ(resultPath, androidQtGetLibraryDir(64).c_str());
}


TEST(androidQtGetLibraryDir, DetectBittness) {
#if _WIN32
    const char basePath[]   = "\\foo";
    const std::string resultPath = std::string("\\foo\\")
                                           .append(System::kLibSubDir)
                                           .append("\\qt\\lib");
#else
    const char basePath[]  = "/foo";
    const std::string resultPath =
            std::string("/foo/").append(System::kLibSubDir).append("/qt/lib");
#endif

    TestSystem testSys(basePath, System::kProgramBitness);

    EXPECT_STREQ(resultPath.c_str(), androidQtGetLibraryDir(0).c_str());
}

TEST(androidQtGetPluginsDir, Qt32) {
#if _WIN32
    const char basePath[]   = "\\foo";
    const char resultPath[] = "\\foo\\lib\\qt\\plugins";
#else
    const char basePath[]  = "/foo";
    const char resultPath[] = "/foo/lib/qt/plugins";
#endif

    TestSystem testSys(basePath, 32);

    EXPECT_STREQ(resultPath, androidQtGetPluginsDir(32).c_str());
}

TEST(androidQtGetPluginsDir, Qt64) {
#if _WIN32
    const char basePath[]   = "\\foo";
    const char resultPath[] = "\\foo\\lib64\\qt\\plugins";
#else
    const char basePath[]  = "/foo";
    const char resultPath[] = "/foo/lib64/qt/plugins";
#endif

    TestSystem testSys(basePath, 64);

    EXPECT_STREQ(resultPath, androidQtGetPluginsDir(64).c_str());
}

TEST(androidQtGetPluginsDir, RespectEnv) {
#if _WIN32
    const char basePath[] = "\\foo";
    const char resultPath[] = "\\other\\qt_plugins";
#else
    const char basePath[]  = "/foo";
    const char resultPath[] = "/other/qt_plugins";
#endif

    TestSystem testSys(basePath, System::kProgramBitness);
    testSys.envSet("ANDROID_QT_QPA_PLATFORM_PLUGIN_PATH", resultPath);

    EXPECT_STREQ(resultPath, androidQtGetPluginsDir(0).c_str());
}

TEST(androidQtGetPluginsDir, DetectBitness) {
#if _WIN32
    const char basePath[] = "\\foo";
    const std::string resultPath = std::string("\\foo\\")
                                           .append(System::kLibSubDir)
                                           .append("\\qt\\plugins");
#else
    const char basePath[]  = "/foo";
    const std::string resultPath = std::string("/foo/")
                                           .append(System::kLibSubDir)
                                           .append("/qt/plugins");
#endif

    TestSystem testSys(basePath, System::kProgramBitness);

    EXPECT_STREQ(resultPath.c_str(), androidQtGetPluginsDir(0).c_str());
}
