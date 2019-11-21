// Copyright 2017 The Android Open Source Project
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
#include "android/utils/path.h"
#include "android/base/testing/TestSystem.h"

#include <gtest/gtest.h>

using namespace android::base;

TEST(AvdUtilWrapper, emulator_path_getSdkRoot) {
  TestSystem sys("", 32, "/myhome");
  ASSERT_TRUE(sys.getTempRoot()->makeSubDir("/tmp"));
  ASSERT_TRUE(sys.getTempRoot()->makeSubDir("/tmp/Sdk"));
  ASSERT_TRUE(sys.getTempRoot()->makeSubDir("/tmp/Sdk/platform-tools"));
  ASSERT_TRUE(sys.getTempRoot()->makeSubDir("/tmp/Sdk/platforms"));
  ASSERT_TRUE(sys.getTempRoot()->makeSubDir("/tmp/Sdk/tools"));
  ASSERT_TRUE(sys.pathIsDir("/tmp/Sdk"));

  sys.setLauncherDirectory(".");
  sys.setCurrentDirectoryForTesting("/tmp/Sdk/tools");

  char* sdk = path_getSdkRoot();
  EXPECT_TRUE(sdk != nullptr);
  EXPECT_TRUE(path_is_absolute(sdk));
  free(sdk);
}


