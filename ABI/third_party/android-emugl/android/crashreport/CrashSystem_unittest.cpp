// Copyright 2015 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "android/crashreport/CrashSystem.h"

#include "android/base/testing/TestSystem.h"
#include "android/base/testing/TestTempDir.h"

#include <gtest/gtest.h>

using namespace android::base;
using namespace android::crashreport;

// TODO fix validatePaths_nonWritableHomeDir
// TODO windows tests

class CrashSystemTest: public ::testing::Test {
public:
    CrashSystemTest() : mTestSystem("/progdir", System::kProgramBitness,
                                     "/homedir", "/appdir"),
                        mTestDir(mTestSystem.getTempRoot()),
                        mCrashServiceName() {
        EXPECT_TRUE(mTestDir->makeSubDir("progdir"));
        EXPECT_TRUE(mTestDir->makeSubDir("homedir"));
        EXPECT_TRUE(mTestDir->makeSubDir("appdir"));
        if (System::kProgramBitness == 32) {
            mCrashServiceName = "emulator-crash-service";
        } else {
            mCrashServiceName = "emulator64-crash-service";
        }
#ifdef _WIN32
        mCrashServiceName+=".exe";
#endif
    }

    TestTempDir* getTestDir() {
        return mTestDir;
    }
    void makeCaBundle() {
        EXPECT_TRUE(mTestDir->makeSubDir("progdir/lib"));
        EXPECT_TRUE(mTestDir->makeSubFile("progdir/lib/ca-bundle.pem"));
    }

    std::string getCrashServiceTestDirPath() {
        std::string crashpath ("progdir");
        crashpath += System::kDirSeparator;
        crashpath += mCrashServiceName;
        return crashpath;
    }

    std::string getCrashServicePath() {
        std::string crashpath ("/");
        crashpath += getCrashServiceTestDirPath();
        return crashpath;
    }

    void makeCrashService() {
        EXPECT_TRUE(mTestDir->makeSubFile(getCrashServiceTestDirPath().c_str()));
    }

    void makeAndroidHome() {
        EXPECT_TRUE(mTestDir->makeSubDir("homedir/.android"));
    }

    void makeBreakpadHome() {
        makeAndroidHome();
        EXPECT_TRUE(mTestDir->makeSubDir("homedir/.android/breakpad"));
    }

private:
    android::base::TestSystem mTestSystem;
    TestTempDir* mTestDir;
    std::string mCrashServiceName;
};


TEST_F(CrashSystemTest, getCrashDirectory) {
    std::string tmp1 = CrashSystem::get()->getCrashDirectory();
#ifdef _WIN32
    EXPECT_STREQ(tmp1.c_str(), "/homedir\\.android\\breakpad");
#else
    EXPECT_STREQ(tmp1.c_str(), "/homedir/.android/breakpad");
#endif
}

TEST_F(CrashSystemTest, getCaBundlePath) {
    std::string tmp1 = CrashSystem::get()->getCaBundlePath();
#ifdef _WIN32
    EXPECT_STREQ(tmp1.c_str(), "/progdir\\lib\\ca-bundle.pem");
#else
    EXPECT_STREQ(tmp1.c_str(), "/progdir/lib/ca-bundle.pem");
#endif
}

TEST_F(CrashSystemTest, getCrashServicePath) {
    std::string tmp1 = CrashSystem::get()->getCrashServicePath();
    std::string crashpath = getCrashServicePath();
    EXPECT_STREQ(tmp1.c_str(), crashpath.c_str());
}

TEST_F(CrashSystemTest, getCrashServiceCmdLine) {
    std::string pipe("pipeval");
    std::string proc("procval");
    std::vector<std::string> tmp1 =
            CrashSystem::get()->getCrashServiceCmdLine(pipe, proc);
    EXPECT_EQ(tmp1.size(), 7U);
    EXPECT_STREQ(tmp1[0].c_str(), getCrashServicePath().c_str());
    EXPECT_STREQ(tmp1[1].c_str(), "-pipe");
    EXPECT_STREQ(tmp1[2].c_str(), pipe.c_str());
    EXPECT_STREQ(tmp1[3].c_str(), "-ppid");
    EXPECT_STREQ(tmp1[4].c_str(), proc.c_str());
    EXPECT_STREQ(tmp1[5].c_str(), "-data-dir");
    // check that argument of -data-dir starts with temp directory name
    const auto tempDir = System::get()->getTempDir();
    EXPECT_TRUE(tmp1[6].size() > tempDir.size() + 2U);
    EXPECT_EQ(std::string(tmp1[6].c_str(), tempDir.size()), tempDir);
    EXPECT_TRUE(PathUtils::isDirSeparator(tmp1[6][tempDir.size()]));
    EXPECT_FALSE(PathUtils::isDirSeparator(tmp1[6][tempDir.size() + 1]));
}

TEST_F(CrashSystemTest, validatePaths_ValidPaths) {
    makeCaBundle();
    makeCrashService();
    makeBreakpadHome();
    EXPECT_TRUE(CrashSystem::get()->validatePaths());
}

TEST_F(CrashSystemTest, validatePaths_NoCrashServiceFile) {
    makeCaBundle();
    makeBreakpadHome();
    EXPECT_FALSE(CrashSystem::get()->validatePaths());
}

TEST_F(CrashSystemTest, validatePaths_CrashServiceIsDir) {
    EXPECT_TRUE(getTestDir()->makeSubDir(getCrashServiceTestDirPath().c_str()));
    makeBreakpadHome();
    EXPECT_FALSE(CrashSystem::get()->validatePaths());
}

TEST_F(CrashSystemTest, validatePaths_NoCaBundle) {
    makeCrashService();
    makeBreakpadHome();
    EXPECT_FALSE(CrashSystem::get()->validatePaths());
}

TEST_F(CrashSystemTest, validatePaths_CaBundleIsDir) {
    EXPECT_TRUE(getTestDir()->makeSubDir("progdir/lib"));
    EXPECT_TRUE(getTestDir()->makeSubDir("progdir/lib/ca-bundle.pem"));
    makeCrashService();
    makeBreakpadHome();
    EXPECT_FALSE(CrashSystem::get()->validatePaths());
}

TEST_F(CrashSystemTest, validatePaths_crashDirIsFile) {
    makeCaBundle();
    makeCrashService();
    makeAndroidHome();
    EXPECT_TRUE(getTestDir()->makeSubFile("homedir/.android/breakpad"));
    EXPECT_FALSE(CrashSystem::get()->validatePaths());
}

//  This test is a false pass.  Currently validatePaths calls a
//    utils/paths::path_mkdir_if_needed function to create the breakpad
//    directory if not already existing.
//  This function does not use the System/TestSystem infrastructure and does
//    ignores the testroot, instead trying to mkdir at the system root "/".
//    This has the same result as if the HOME directory path was not writable.
//  TEST(CrashSystemTest, validatePaths_nonWritableHomeDir) {
//       makeCaBundle();
//       makeCrashService();
//       EXPECT_FALSE(CrashSystem::get()->validatePaths());
//}
