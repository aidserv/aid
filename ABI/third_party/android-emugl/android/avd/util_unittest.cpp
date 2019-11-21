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

#include "android/avd/util.h"
#include "android/base/testing/TestSystem.h"
#include "android/utils/file_data.h"
#include "android/base/ArraySize.h"
#include "android/base/testing/TestSystem.h"

#include <iostream>
#include <fstream>
#include <memory>
#include <gtest/gtest.h>
#include "android/base/testing/TestTempDir.h"


using android::base::TestSystem;
using android::base::TestTempDir;

static void writeToFile(std::string path, std::string text) {
    std::ofstream iniFile(path, std::ios::trunc);
    iniFile << text;
    iniFile.close();
}

TEST(AvdUtil, path_getAvdSystemPath) {
    TestSystem sys("/home", 64, "/");
    TestTempDir* tmp = sys.getTempRoot();
    tmp->makeSubDir("android_home");
    tmp->makeSubDir("android_home/sysimg");
    tmp->makeSubDir("android_home/avd");
    tmp->makeSubDir("nothome");

    std::string sdkRoot = tmp->pathString() + "/android_home";
    std::string avdConfig = sdkRoot + "/avd/config.ini";
    sys.envSet("ANDROID_AVD_HOME", sdkRoot);

    // Create an in file for the @q avd.
    writeToFile(sdkRoot + "/q.ini", "path=" + sdkRoot + "/avd");

    // A relative path should be resolved from ANRDOID_AVD_HOME
    writeToFile(avdConfig, "image.sysdir.1=sysimg");

    std::unique_ptr<char[]> path(path_getAvdSystemPath("q", sdkRoot.c_str()));
    EXPECT_STREQ((sdkRoot + "/sysimg").c_str(), path.get());

    // An absolute path should be usuable as well
    writeToFile(avdConfig,
                 "image.sysdir.1=" + tmp->pathString() + "/nothome");

    path.reset(path_getAvdSystemPath("q", sdkRoot.c_str()));
    EXPECT_STREQ((tmp->pathString() + "/nothome").c_str(), path.get());

    std::string noBufferOverflow(MAX_PATH * 2, 'Z');
    writeToFile(avdConfig, "image.sysdir.1=" + noBufferOverflow);
    path.reset(path_getAvdSystemPath("q", sdkRoot.c_str()));
    EXPECT_EQ(nullptr, path.get());
}

TEST(AvdUtil, emulator_getBackendSuffix) {
  EXPECT_STREQ("arm", emulator_getBackendSuffix("arm"));
  EXPECT_STREQ("x86", emulator_getBackendSuffix("x86"));
  EXPECT_STREQ("x86", emulator_getBackendSuffix("x86_64"));
  EXPECT_STREQ("mips", emulator_getBackendSuffix("mips"));
  EXPECT_STREQ("arm64", emulator_getBackendSuffix("arm64"));
  EXPECT_STREQ("mips64", emulator_getBackendSuffix("mips64"));

  EXPECT_FALSE(emulator_getBackendSuffix(NULL));
  EXPECT_FALSE(emulator_getBackendSuffix("dummy"));
}

TEST(AvdUtil, propertyFile_getInt) {
  FileData fd;

  const char* testFile =
    "nineteen=19\n"
    "int_min=-2147483648\n"
    "int_max=2147483647\n"
    "invalid=2147483648\n"
    "invalid2=-2147483649\n"
    "invalid3=bar\n"
    "empty=\n";

  EXPECT_EQ(0,fileData_initFromMemory(&fd, testFile, strlen(testFile)));

  const int kDefault = 1138;
  SearchResult kSearchResultGarbage = (SearchResult)0xdeadbeef;
  SearchResult searchResult = kSearchResultGarbage;

  EXPECT_EQ(kDefault,propertyFile_getInt(&fd, "invalid", kDefault, &searchResult));
  EXPECT_EQ(RESULT_INVALID,searchResult);

  searchResult = kSearchResultGarbage;
  EXPECT_EQ(kDefault,propertyFile_getInt(&fd, "invalid2", kDefault, &searchResult));
  EXPECT_EQ(RESULT_INVALID,searchResult);

  searchResult = kSearchResultGarbage;
  EXPECT_EQ(kDefault,propertyFile_getInt(&fd, "invalid3", kDefault, &searchResult));
  EXPECT_EQ(RESULT_INVALID,searchResult);

  searchResult = kSearchResultGarbage;
  EXPECT_EQ(kDefault,propertyFile_getInt(&fd, "bar", kDefault, &searchResult));
  EXPECT_EQ(RESULT_NOT_FOUND,searchResult);

  searchResult = kSearchResultGarbage;
  EXPECT_EQ(kDefault,propertyFile_getInt(&fd, "empty", kDefault, &searchResult));
  EXPECT_EQ(RESULT_INVALID,searchResult);

  searchResult = kSearchResultGarbage;
  EXPECT_EQ(19,propertyFile_getInt(&fd, "nineteen", kDefault, &searchResult));
  EXPECT_EQ(RESULT_FOUND,searchResult);

  // check that null "searchResult" parameter is supported
  EXPECT_EQ(kDefault,propertyFile_getInt(&fd, "bar", kDefault, NULL));
  EXPECT_EQ(kDefault,propertyFile_getInt(&fd, "invalid", kDefault, NULL));
  EXPECT_EQ(19,propertyFile_getInt(&fd, "nineteen", kDefault, NULL));
}

TEST(AvdUtil, propertyFile_getApiLevel) {
  FileData fd;

  const char* emptyFile =
    "\n";

  const char* testFile19 =
    "ro.build.version.sdk=19\n";

  const char* testFileBogus =
    "ro.build.version.sdk=bogus\n";

  EXPECT_EQ(0,fileData_initFromMemory(&fd, emptyFile, strlen(emptyFile)));
  EXPECT_EQ(10000,propertyFile_getApiLevel(&fd));

  EXPECT_EQ(0,fileData_initFromMemory(&fd, testFile19, strlen(testFile19)));
  EXPECT_EQ(19,propertyFile_getApiLevel(&fd));

  EXPECT_EQ(0,fileData_initFromMemory(&fd, testFileBogus, strlen(testFileBogus)));
  EXPECT_EQ(3,propertyFile_getApiLevel(&fd));
}

TEST(AvdUtil, propertyFile_findProductName) {
  FileData fd;

  const char* testFileGoogle =
    "ro.product.name=sdk_google_x86-eng\n";

  const char* testFileGoogle2 =
      "ro.product.name=google_sdk_x86-eng\n";

  const char* testFilePhone =
    "ro.product.name=abc_phone_x86-eng\n";

  const char* testFileAndroidAuto =
      "ro.product.name=car_emu_x86_64-userdebug\n";

  const char* testFileRandom =
      "ro.product.name=bat_land-userdebug\n";

  EXPECT_EQ(0, fileData_initFromMemory(&fd, testFileGoogle, strlen(testFileGoogle)));
  const char *google_names[] = {"sdk_google", "google_sdk"};
  EXPECT_TRUE(propertyFile_findProductName(&fd, google_names, ARRAY_SIZE(google_names), false));

  EXPECT_EQ(0, fileData_initFromMemory(&fd, testFileGoogle2, strlen(testFileGoogle2)));
  EXPECT_TRUE(propertyFile_findProductName(&fd, google_names, ARRAY_SIZE(google_names), false));

  EXPECT_EQ(0, fileData_initFromMemory(&fd, testFilePhone, strlen(testFilePhone)));
  const char *phone_names[] = {"phone"};
  EXPECT_TRUE(propertyFile_findProductName(&fd, phone_names, ARRAY_SIZE(phone_names), false));

  EXPECT_EQ(0, fileData_initFromMemory(&fd, testFileAndroidAuto, strlen(testFileAndroidAuto)));
  const char *car_names[] = {"car_emu"};
  EXPECT_TRUE(propertyFile_findProductName(&fd, car_names, ARRAY_SIZE(car_names), true));

  EXPECT_EQ(0, fileData_initFromMemory(&fd, testFileRandom, strlen(testFileRandom)));
  EXPECT_FALSE(propertyFile_findProductName(&fd, google_names, ARRAY_SIZE(google_names), false));
  EXPECT_FALSE(propertyFile_findProductName(&fd, phone_names, ARRAY_SIZE(phone_names), false));
  EXPECT_FALSE(propertyFile_findProductName(&fd, car_names, ARRAY_SIZE(car_names), true));
}
