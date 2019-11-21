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

#include "android/windows_installer.h"

#include "android/base/system/System.h"

#include <gtest/gtest.h>
#include <stdio.h>

namespace android {

namespace {

std::string remindForWine(void) {
    if (!base::System::get()->isRunningUnderWine()) {
        return "";
    }
    return "Have you ever run android/tests/prepare-registry-for-wine.sh?";
}

}  // namespace

TEST(WindowsInstaller, getVersion) {
    EXPECT_EQ(WindowsInstaller::kNotInstalled,
              WindowsInstaller::getVersion("Google Play")) << remindForWine();

    // I know of no package that can be depended on across all Windows versions
    // especially considering Wine
    // not really a unit test, visually inspect result
    int32_t version = WindowsInstaller::getVersion(u8"Intel® Hardware Accelerated Execution Manager");
    printf("HAXM version: %08x\n", version);
}

}  // namespace android

