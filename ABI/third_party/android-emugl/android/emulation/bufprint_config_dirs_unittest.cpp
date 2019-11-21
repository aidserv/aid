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

#include "android/emulation/bufprint_config_dirs.h"

#include "android/base/system/System.h"
#include "android/base/testing/TestSystem.h"
#include "android/base/testing/TestTempDir.h"
#include "android/utils/path.h"

#include <gtest/gtest.h>

using namespace android::base;

TEST(bufprint, ApplicationDirectory) {
    TestSystem sys("/bin", 32);
    char buffer[32], *p = buffer, *end = buffer + sizeof(buffer);
    p = bufprint_app_dir(buffer, end);
    EXPECT_EQ(buffer + 4, p);
    EXPECT_STREQ("/bin", buffer);
}

TEST(bufprint, AvdHomePathDefault) {
    TestSystem sys("/bin", 32);
    char buffer[32], *p = buffer, *end = buffer + sizeof(buffer);
    p = bufprint_avd_home_path(p, end);
    EXPECT_EQ(buffer + 18, p);
#ifdef _WIN32
    EXPECT_STREQ("/home\\.android\\avd", buffer);
#else
    EXPECT_STREQ("/home/.android/avd", buffer);
#endif
}

TEST(bufprint, AvdHomePathWithAndroidAvdHome) {
    TestSystem sys("/bin", 32, "/");
    ASSERT_TRUE(sys.getTempRoot()->makeSubDir("sdksdk"));
    ASSERT_TRUE(sys.getTempRoot()->makeSubDir("sdksdk/.android"));
    ASSERT_TRUE(sys.getTempRoot()->makeSubDir("sdksdk/.android/avd"));
    sys.envSet("ANDROID_AVD_HOME", "sdksdk/.android/avd");
    char buffer[32], *p = buffer, *end = buffer + sizeof(buffer);
    p = bufprint_avd_home_path(p, end);
    EXPECT_EQ(buffer + 19, p);
    EXPECT_STREQ("sdksdk/.android/avd", buffer);
}

TEST(bufprint, ConfigPathDefault) {
    TestSystem sys("/bin", 32);
    char buffer[32], *p = buffer, *end = buffer + sizeof(buffer);
    p = bufprint_config_path(p, end);
    EXPECT_EQ(buffer + 14, p);
#ifdef _WIN32
    EXPECT_STREQ("/home\\.android", buffer);
#else
    EXPECT_STREQ("/home/.android", buffer);
#endif
}

TEST(bufprint, ConfigPathWithAndroidEmulatorHome) {
    TestSystem sys("/bin", 32);
    sys.envSet("ANDROID_EMULATOR_HOME", "/myhome");
    char buffer[32], *p = buffer, *end = buffer + sizeof(buffer);
    p = bufprint_config_path(buffer, end);
    EXPECT_EQ(buffer + 7, p);
    EXPECT_STREQ("/myhome", buffer);
}

TEST(bufprint, ConfigPathWithAndroidSdkHome) {
    TestSystem sys("/bin", 32, "/");
    sys.envSet("ANDROID_SDK_HOME", "/sdk-home");
    char buffer[32], *p = buffer, *end = buffer + sizeof(buffer);
    p = bufprint_config_path(p, end);
    EXPECT_EQ(buffer + 9, p);
    EXPECT_STREQ("/sdk-home", buffer);

    ASSERT_TRUE(sys.getTempRoot()->makeSubDir("sdk-home"));
    ASSERT_TRUE(sys.getTempRoot()->makeSubDir("sdk-home/.android"));

    p = bufprint_config_path(buffer, end);
    EXPECT_EQ(buffer + 18, p);
#ifdef _WIN32
    EXPECT_STREQ("/sdk-home\\.android", buffer);
#else
    EXPECT_STREQ("/sdk-home/.android", buffer);
#endif
}

TEST(bufprint, ConfigFile) {
    TestSystem sys("/bin", 32);
    char buffer[32], *p = buffer, *end = buffer + sizeof(buffer);
    p = bufprint_config_file(p, end, "file");
    EXPECT_EQ(buffer + 19, p);
#ifdef _WIN32
    EXPECT_STREQ("/home\\.android\\file", buffer);
#else
    EXPECT_STREQ("/home/.android/file", buffer);
#endif
}
