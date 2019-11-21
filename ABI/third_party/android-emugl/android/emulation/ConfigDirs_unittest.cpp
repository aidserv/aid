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

#include "android/emulation/ConfigDirs.h"

#include "android/base/files/PathUtils.h"
#include "android/base/testing/TestSystem.h"
#include "android/utils/path.h"

#include <gtest/gtest.h>

using android::ConfigDirs;
using namespace android::base;


TEST(ConfigDirs, getUserDirectoryDefault) {
    TestSystem sys(PATH_SEP "bin", 32, PATH_SEP "myhome");
    static const char kExpected[] = PATH_SEP "myhome" PATH_SEP ".android";
    EXPECT_STREQ(kExpected, ConfigDirs::getUserDirectory().c_str());
}

TEST(ConfigDirs, getUserDirectoryWithAndroidSdkHome) {
    TestSystem sys(PATH_SEP "bin", 32, PATH_SEP "myhome");
    sys.envSet("ANDROID_SDK_HOME", PATH_SEP "android-sdk");
    EXPECT_STREQ(PATH_SEP "android-sdk", ConfigDirs::getUserDirectory().c_str());

    sys.getTempRoot()->makeSubDir("android-sdk");
    sys.getTempRoot()->makeSubDir("android-sdk" PATH_SEP ".android");
    EXPECT_STREQ(PathUtils::join(PATH_SEP "android-sdk", ".android").c_str(),
                 ConfigDirs::getUserDirectory().c_str());
}

TEST(ConfigDirs, getUserDirectoryWithAndroidEmulatorHome) {
    TestSystem sys(PATH_SEP "bin", 32, PATH_SEP "myhome");
    sys.envSet("ANDROID_EMULATOR_HOME", PATH_SEP "android" PATH_SEP "home");
    EXPECT_STREQ(PATH_SEP "android" PATH_SEP "home", ConfigDirs::getUserDirectory().c_str());
}


TEST(ConfigDirs, getSdkRootDirectory) {
    TestSystem sys("", 32, PATH_SEP "myhome");
    ASSERT_TRUE(sys.getTempRoot()->makeSubDir("Sdk"));
    ASSERT_TRUE(sys.getTempRoot()->makeSubDir("Sdk" PATH_SEP "platform-tools"));
    ASSERT_TRUE(sys.getTempRoot()->makeSubDir("Sdk" PATH_SEP "platforms"));
    ASSERT_TRUE(sys.pathIsDir(PATH_SEP "Sdk"));

    sys.envSet("ANDROID_SDK_ROOT", PATH_SEP "Sdk");
    EXPECT_STREQ(PATH_SEP "Sdk", ConfigDirs::getSdkRootDirectory().c_str());

    sys.envSet("ANDROID_SDK_ROOT", "\"" PATH_SEP "Sdk\"");
    EXPECT_STREQ(PATH_SEP "Sdk", ConfigDirs::getSdkRootDirectory().c_str());

    sys.envSet("ANDROID_SDK_ROOT", "");
    EXPECT_STRNE("Sdk", ConfigDirs::getSdkRootDirectory().c_str());

    ASSERT_TRUE(sys.getTempRoot()->makeSubDir("Sdk2"));
    ASSERT_TRUE(sys.getTempRoot()->makeSubDir("Sdk2" PATH_SEP "platform-tools"));
    ASSERT_TRUE(sys.getTempRoot()->makeSubDir("Sdk2" PATH_SEP "platforms"));
    ASSERT_TRUE(sys.pathIsDir(PATH_SEP "Sdk2"));

    // ANDROID_HOME should take precedence over ANDROID_SDK_ROOT
    sys.envSet("ANDROID_HOME", PATH_SEP "Sdk2");
    EXPECT_STREQ(PATH_SEP "Sdk2", ConfigDirs::getSdkRootDirectory().c_str());

    // Bad ANDROID_HOME falls back to ANDROID_SDK_ROOT
    sys.envSet("ANDROID_HOME", PATH_SEP "bogus");
    sys.envSet("ANDROID_SDK_ROOT", PATH_SEP "Sdk");
    EXPECT_STREQ(PATH_SEP "Sdk", ConfigDirs::getSdkRootDirectory().c_str());
}


TEST(ConfigDirs, getAvdRootDirectory) {
    TestSystem sys("", 32, PATH_SEP "myhome");
    ASSERT_TRUE(sys.getTempRoot()->makeSubDir("Area_1"));
    ASSERT_TRUE(sys.getTempRoot()->makeSubDir("Area_1" PATH_SEP ".android"));
    ASSERT_TRUE(sys.getTempRoot()->makeSubDir("Area_1" PATH_SEP ".android" PATH_SEP "avd"));
    ASSERT_TRUE(sys.getTempRoot()->makeSubDir("Area_2"));
    ASSERT_TRUE(sys.getTempRoot()->makeSubDir("Area_2" PATH_SEP ".android"));
    ASSERT_TRUE(sys.getTempRoot()->makeSubDir("Area_2" PATH_SEP ".android" PATH_SEP "avd"));
    ASSERT_TRUE(sys.getTempRoot()->makeSubDir("Area_3"));
    ASSERT_TRUE(sys.getTempRoot()->makeSubDir("Area_3" PATH_SEP ".android"));
    ASSERT_TRUE(sys.getTempRoot()->makeSubDir("Area_3" PATH_SEP ".android" PATH_SEP "avd"));
    ASSERT_TRUE(sys.getTempRoot()->makeSubDir("Area_4"));
    ASSERT_TRUE(sys.getTempRoot()->makeSubDir("Area_4" PATH_SEP ".android"));
    ASSERT_TRUE(sys.getTempRoot()->makeSubDir("Area_4" PATH_SEP ".android" PATH_SEP "avd"));
    ASSERT_TRUE(sys.pathIsDir(PATH_SEP "Area_1" PATH_SEP ".android" PATH_SEP "avd"));
    ASSERT_TRUE(sys.pathIsDir(PATH_SEP "Area_2" PATH_SEP ".android" PATH_SEP "avd"));
    ASSERT_TRUE(sys.pathIsDir(PATH_SEP "Area_3" PATH_SEP ".android" PATH_SEP "avd"));
    ASSERT_TRUE(sys.pathIsDir(PATH_SEP "Area_4" PATH_SEP ".android" PATH_SEP "avd"));

    // Order of precedence is
    //   ANDROID_AVD_HOME
    //   ANDROID_SDK_HOME
    //   USER_HOME or HOME
    //   ANDROID_EMULATOR_HOME

    sys.envSet("ANDROID_AVD_HOME", PATH_SEP "Area_1" PATH_SEP ".android" PATH_SEP "avd");
    sys.envSet("ANDROID_SDK_HOME", PATH_SEP "Area_2");
    sys.envSet("USER_HOME", PATH_SEP "Area_3");
    sys.envSet("ANDROID_EMULATOR_HOME", PATH_SEP "Area_4" PATH_SEP ".android");
    EXPECT_STREQ(PATH_SEP "Area_1" PATH_SEP ".android" PATH_SEP "avd", ConfigDirs::getAvdRootDirectory().c_str());

    sys.envSet("ANDROID_AVD_HOME", "bogus");
    EXPECT_STREQ(PATH_SEP "Area_2" PATH_SEP ".android" PATH_SEP "avd", ConfigDirs::getAvdRootDirectory().c_str());

    sys.envSet("ANDROID_SDK_HOME", "bogus");
    EXPECT_STREQ(PATH_SEP "Area_3" PATH_SEP ".android" PATH_SEP "avd", ConfigDirs::getAvdRootDirectory().c_str());

    sys.envSet("USER_HOME", "bogus");
    EXPECT_STREQ(PATH_SEP "Area_4" PATH_SEP ".android" PATH_SEP "avd", ConfigDirs::getAvdRootDirectory().c_str());
}
