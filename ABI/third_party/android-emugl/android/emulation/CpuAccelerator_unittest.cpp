// Copyright (C) 2014 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/emulation/CpuAccelerator.h"
#include "android/emulation/internal/CpuAccelerator.h"

#include <gtest/gtest.h>

#include <string>

#include <stdio.h>

namespace android {

class CpuAcceleratorTest : public ::testing::Test {
public:
    CpuAcceleratorTest() {
        saved_accel_ = GetCurrentCpuAccelerator();
        saved_status_ = GetCurrentCpuAcceleratorStatus();
    }

    ~CpuAcceleratorTest() {
        // Restore previous state.
        SetCurrentCpuAcceleratorForTesting(saved_accel_,
                                           saved_status_code_,
                                           saved_status_.c_str());
    }
private:
    CpuAccelerator saved_accel_;
    AndroidCpuAcceleration saved_status_code_;
    std::string saved_status_;
};

// Not really a test, but a simple way to print the current accelerator
// value for simple verification.
TEST_F(CpuAcceleratorTest, Default) {
    CpuAccelerator accel = GetCurrentCpuAccelerator();
    std::string status = GetCurrentCpuAcceleratorStatus();

    switch (accel) {
    case CPU_ACCELERATOR_NONE:
        printf("No acceleration possible on this machine!\n");
        break;

    case CPU_ACCELERATOR_KVM:
        printf("KVM acceleration usable on this machine!\n");
        break;

    case CPU_ACCELERATOR_HAX:
        printf("HAX acceleration usable on this machine!\n");
        break;

    default:
        ASSERT_FALSE(1) << "Invalid accelerator value: " << accel;
    }
    printf("Status: %s\n", status.c_str());
}

TEST(CpuAccelerator, GetCpuInfo) {
    auto info = android::GetCpuInfo();
    EXPECT_NE(info.first, ANDROID_CPU_INFO_FAILED);
    EXPECT_GT(info.second.size(), 0U);
}

#ifdef __APPLE__

TEST(cpuAcceleratorGetHaxVersion, Test) {
    const char* kext_dir[] = {
        "this-directory-does-not-exist",
        "android", // this directory exists but doesn't contain the file
        "android/android-emu/android/emulation",
    };

    // this is a real version from from HAXM 1.2.1
    ASSERT_EQ(0x01020001, cpuAcceleratorGetHaxVersion(kext_dir, 3, "CpuAccelerator_unittest.dat2"));

    // this is a real version from from HAXM 1.1.4
    const char* version_file = "CpuAccelerator_unittest.dat";

    ASSERT_EQ(0x01010004, cpuAcceleratorGetHaxVersion(kext_dir, 3, version_file));

    // only looking in the first directory, won't be found
    ASSERT_EQ(0, cpuAcceleratorGetHaxVersion(kext_dir, 1, version_file));

    // the second directory will be found but the version file will be missing
    ASSERT_EQ(-1, cpuAcceleratorGetHaxVersion(kext_dir, 2, version_file));

    // this file will have "VERSION=" but not a valid number following it
    ASSERT_EQ(-1, cpuAcceleratorGetHaxVersion(kext_dir, 3, "CpuAccelerator_unittest.cpp"));
}

TEST(cpuAcceleratorParseVersionScript, Test) {
    ASSERT_EQ(0x01020004, cpuAcceleratorParseVersionScript("VERSION=1.2.4"));
    ASSERT_EQ(0x0203000a, cpuAcceleratorParseVersionScript("VERSION=2.3.10\r"));
    ASSERT_EQ(0x04010000, cpuAcceleratorParseVersionScript("VERSION=4.1"));
    ASSERT_EQ(0x03000000, cpuAcceleratorParseVersionScript("VERSION=3\n"));
    ASSERT_EQ(0x7fffffff, cpuAcceleratorParseVersionScript("VERSION=127.255.65535"));
    ASSERT_EQ(-1, cpuAcceleratorParseVersionScript("VERSION=.1"));
    ASSERT_EQ(-1, cpuAcceleratorParseVersionScript("VERSION=128.0.0"));
    ASSERT_EQ(-1, cpuAcceleratorParseVersionScript("VERSION=0"));
    ASSERT_EQ(-1, cpuAcceleratorParseVersionScript("VERSION=1.256.3"));
    ASSERT_EQ(-1, cpuAcceleratorParseVersionScript("VERSION=1.2.65536"));
    ASSERT_EQ(-1, cpuAcceleratorParseVersionScript("VERSION=\n"));
    ASSERT_EQ(-1, cpuAcceleratorParseVersionScript("asdf"));
    ASSERT_EQ(-1, cpuAcceleratorParseVersionScript(""));
}

#endif // __APPLE__

}  // namespace android
