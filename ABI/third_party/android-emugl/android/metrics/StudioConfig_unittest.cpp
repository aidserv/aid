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

#include "android/metrics/StudioConfig.h"

#include "android/base/testing/TestSystem.h"
#include "android/base/testing/TestTempDir.h"
#include "android/base/Version.h"
#include "android/utils/path.h"

#include <gtest/gtest.h>

#include <fstream>

/***************************************************************************/
// These macros are replicated as consts in StudioConfig.cpp
// changes to these will require equivalent changes to the unittests
//

#ifdef __APPLE__
#define ANDROID_STUDIO_DIR "AndroidStudio"
#else  // ! ___APPLE__
#define ANDROID_STUDIO_DIR ".AndroidStudio"
#define ANDROID_STUDIO_DIR_INFIX "config"
#endif  // __APPLE__

#define ANDROID_STUDIO_DIR_SUFFIX "options"
#define ANDROID_STUDIO_DIR_PREVIEW "Preview"

using namespace android;
using namespace android::base;
using namespace android::studio;
using std::endl;
using std::ofstream;

TEST(StudioConfigTest, androidStudioVersioning) {
    const char* str = NULL;

    str = ANDROID_STUDIO_DIR;
    Version v1studio(extractAndroidStudioVersion(str));
    Version v1 = Version(0, 0, 0, 2);
    EXPECT_EQ(v1studio, v1);

    str = ANDROID_STUDIO_DIR ANDROID_STUDIO_DIR_PREVIEW;
    Version v2studio(extractAndroidStudioVersion(str));
    Version v2 = Version(0, 0, 0, 1);
    EXPECT_EQ(v2studio, v2);

    str = "prefix_" ANDROID_STUDIO_DIR;
    Version v3studio(extractAndroidStudioVersion(str));
    Version v3 = Version::invalid();
    EXPECT_EQ(v3studio, v3);

    str = ANDROID_STUDIO_DIR ANDROID_STUDIO_DIR_PREVIEW "_suffix";
    Version v4studio(extractAndroidStudioVersion(str));
    Version v4 = Version::invalid();
    EXPECT_EQ(v4studio, v4);

    str = ANDROID_STUDIO_DIR "1";
    Version v5studio(extractAndroidStudioVersion(str));
    Version v5 = Version(1, 0, 0, 2);
    EXPECT_EQ(v5studio, v5);

    str = ANDROID_STUDIO_DIR ANDROID_STUDIO_DIR_PREVIEW "1.3";
    Version v6studio(extractAndroidStudioVersion(str));
    Version v6 = Version(1, 3, 0, 1);
    EXPECT_EQ(v6studio, v6);

    str = ANDROID_STUDIO_DIR "1.20.3";
    Version v7studio(extractAndroidStudioVersion(str));
    Version v7 = Version(1, 20, 3, 2);
    EXPECT_EQ(v7studio, v7);
}

#ifdef _WIN32
#define FS "\\"
#else
#define FS "/"
#endif

TEST(StudioConfigTest, androidStudioScanner) {
    TestSystem testSys("/root", 32, "/root/home");
    TestTempDir* testDir = testSys.getTempRoot();

    std::string studioPath;
    std::string foundStudioPath;

    testDir->makeSubDir("root");
    testDir->makeSubDir("root/home");
    testDir->makeSubDir("root/adir");
    testDir->makeSubFile("root/afile");

    foundStudioPath = latestAndroidStudioDir("/root");
    EXPECT_STREQ(foundStudioPath.c_str(), studioPath.c_str());

    testDir->makeSubFile("root/home/afile");
    testDir->makeSubDir("root/home/" ANDROID_STUDIO_DIR "-invalid");
    testDir->makeSubDir("root/home/" ANDROID_STUDIO_DIR);

    studioPath = "/root/home" FS ANDROID_STUDIO_DIR;
    foundStudioPath = latestAndroidStudioDir("/root/home");
    EXPECT_STREQ(foundStudioPath.c_str(), studioPath.c_str());

    testDir->makeSubDir("root/home/" ANDROID_STUDIO_DIR "0");
    testDir->makeSubDir("root/home/" ANDROID_STUDIO_DIR "0.1");

    studioPath = "/root/home" FS ANDROID_STUDIO_DIR "0.1";
    foundStudioPath = latestAndroidStudioDir("/root/home");
    EXPECT_STREQ(foundStudioPath.c_str(), studioPath.c_str());

    testDir->makeSubDir("root/home/" ANDROID_STUDIO_DIR "1");
    testDir->makeSubDir(
            "root/home/" ANDROID_STUDIO_DIR ANDROID_STUDIO_DIR_PREVIEW "1");

    studioPath = "/root/home" FS ANDROID_STUDIO_DIR "1";
    foundStudioPath = latestAndroidStudioDir("/root/home");
    EXPECT_STREQ(foundStudioPath.c_str(), studioPath.c_str());

    testDir->makeSubDir("root/home/" ANDROID_STUDIO_DIR "1.2");
    testDir->makeSubDir(
            "root/home/" ANDROID_STUDIO_DIR ANDROID_STUDIO_DIR_PREVIEW "1.2");
    testDir->makeSubFile("root/home/" ANDROID_STUDIO_DIR "1.3");
    testDir->makeSubDir("root/home/prefix" ANDROID_STUDIO_DIR "1.4");

    // 1.3 is a file, not valid
    studioPath = "/root/home" FS ANDROID_STUDIO_DIR "1.2";
    foundStudioPath = latestAndroidStudioDir("/root/home");
    EXPECT_STREQ(foundStudioPath.c_str(), studioPath.c_str());

    testDir->makeSubDir("root/home/" ANDROID_STUDIO_DIR "1.2.3");
    testDir->makeSubDir(
            "root/home/" ANDROID_STUDIO_DIR ANDROID_STUDIO_DIR_PREVIEW "1.2.3");
    testDir->makeSubDir("root/home/" ANDROID_STUDIO_DIR "10.20.30");
    testDir->makeSubDir("root/home/" ANDROID_STUDIO_DIR "Preview10.20.30");

    studioPath = "/root/home" FS ANDROID_STUDIO_DIR "10.20.30";
    foundStudioPath = latestAndroidStudioDir("/root/home");
    EXPECT_STREQ(foundStudioPath.c_str(), studioPath.c_str());
}

TEST(StudioConfigTest, androidStudioXMLPathBuilder) {
    TestSystem testSys("/root", 32, "/root/home", "root/home");
    TestTempDir* testDir = testSys.getTempRoot();

    testDir->makeSubDir("root");
    testDir->makeSubDir("root/home");
    testDir->makeSubDir("root/home/" ANDROID_STUDIO_DIR);
    testDir->makeSubDir("root/home/" ANDROID_STUDIO_DIR "1.4");
    testDir->makeSubDir(
            "root/home/" ANDROID_STUDIO_DIR ANDROID_STUDIO_DIR_PREVIEW "1.4");
    testDir->makeSubFile("root/home/" ANDROID_STUDIO_DIR "10.40");

    std::string foundStudioPath =
            latestAndroidStudioDir("/root/home");
    std::string foundStudioXMLPath = pathToStudioXML(
            foundStudioPath, std::string("usage.statistics.xml"));

#ifdef __APPLE__
    // Normally, path to .AndroidStudio in MacOSX would include
    // /path/to/home/Library/Preferences (equivalent of APPDATA).
    // We simplify this test by setting APPDATA=HOME
    const char* studioXMLPath =
            "/root/home" FS ANDROID_STUDIO_DIR
            "1.4" FS ANDROID_STUDIO_DIR_SUFFIX FS "usage.statistics.xml";
#else
    const char* studioXMLPath =
            "/root/home" FS ANDROID_STUDIO_DIR
            "1.4" FS ANDROID_STUDIO_DIR_INFIX FS ANDROID_STUDIO_DIR_SUFFIX FS
            "usage.statistics.xml";
#endif

    EXPECT_STREQ(foundStudioXMLPath.c_str(), studioXMLPath);
}

// Forward-declare the function that actually calculates the installation ID,
// without using the cached value.
namespace android { namespace studio { std::string extractInstallationId(); }}

TEST(StudioConfigTest, androidStudioInstallationId) {
    TestSystem testSys("/root", 32);
    TestTempDir* testDir = testSys.getTempRoot();
    testSys.setHomeDirectory(std::string(testDir->path()).append("/root/home"));
    testSys.setAppDataDirectory(
            std::string(testDir->path()).append("/root/appdata"));

    testDir->makeSubDir("root");
    testDir->makeSubDir("root/home");
    testDir->makeSubDir("root/home/.android");

    auto noUuid = extractInstallationId();
    EXPECT_STREQ("", noUuid.c_str());

    testDir->makeSubFile("root/home/.android/analytics.settings");

    {
        std::ofstream settings(testDir->pathString() +
                               "/root/home/.android/analytics.settings",
                               std::ios_base::trunc);
        ASSERT_TRUE(!!settings);
        settings << R"("userId":"1 2 3")";
        settings.close();
        auto settingsId = extractInstallationId();
        EXPECT_STREQ("1 2 3", settingsId.c_str());
    }
    {
        std::ofstream settings(testDir->pathString() +
                               "/root/home/.android/analytics.settings",
                               std::ios_base::trunc);
        ASSERT_TRUE(!!settings);
        settings << R"("userId":123)";
        settings.close();
        auto settingsId = extractInstallationId();
        EXPECT_STREQ("123", settingsId.c_str());
    }
    {
        std::ofstream settings(testDir->pathString() +
                               "/root/home/.android/analytics.settings",
                               std::ios_base::trunc);
        ASSERT_TRUE(!!settings);
        settings << R"("userId":1234,something:else)";
        settings.close();
        auto settingsId = extractInstallationId();
        EXPECT_STREQ("1234", settingsId.c_str());
    }
}

TEST(StudioConfigTest, metricsOptIn) {
    TestSystem testSys("/root", 32);
    TestTempDir* testDir = testSys.getTempRoot();
    testSys.setHomeDirectory(std::string(testDir->path()).append("/root/home"));
    testSys.setAppDataDirectory(
            std::string(testDir->path()).append("/root/appdata"));

    testDir->makeSubDir("root");
    testDir->makeSubDir("root/home");
    testDir->makeSubDir("root/home/.android");
    testDir->makeSubFile("root/home/.android/analytics.settings");
    {
        std::ofstream settings(testDir->pathString() +
                               "/root/home/.android/analytics.settings",
                               std::ios_base::trunc);
        ASSERT_TRUE(!!settings);
        settings << R"("hasOptedIn":true)";
        settings.close();
        EXPECT_TRUE(getUserMetricsOptIn());
    }
    {
        std::ofstream settings(testDir->pathString() +
                               "/root/home/.android/analytics.settings",
                               std::ios_base::trunc);
        ASSERT_TRUE(!!settings);
        settings.close();
        EXPECT_FALSE(getUserMetricsOptIn());
    }
    {
        std::ofstream settings(testDir->pathString() +
                               "/root/home/.android/analytics.settings",
                               std::ios_base::trunc);
        ASSERT_TRUE(!!settings);
        settings << R"("hasOptedIn":1)";
        settings.close();
        EXPECT_TRUE(getUserMetricsOptIn());
    }
    {
        std::ofstream settings(testDir->pathString() +
                               "/root/home/.android/analytics.settings",
                               std::ios_base::trunc);
        ASSERT_TRUE(!!settings);
        settings << R"("hasOptedIn":false)";
        settings.close();
        EXPECT_FALSE(getUserMetricsOptIn());
    }
    {
        std::ofstream settings(testDir->pathString() +
                               "/root/home/.android/analytics.settings",
                               std::ios_base::trunc);
        ASSERT_TRUE(!!settings);
        settings << R"("hasOptedIn":blah)";
        settings.close();
        EXPECT_FALSE(getUserMetricsOptIn());
    }
}
