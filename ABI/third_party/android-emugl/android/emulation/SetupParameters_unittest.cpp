// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/emulation/SetupParameters.h"

#include "android/base/StringFormat.h"

#include <gtest/gtest.h>

#include <memory>

namespace android {

namespace {

struct AvdSettings {
    int apiLevel;
    const char* targetArch;
    const char* serialPrefix;
    bool isQemu2;

    std::string toString() const {
        return android::base::StringFormat("api=%d arch=%s qemu2=%d", apiLevel,
                                           targetArch, isQemu2 ? 1 : 0);
    }
};

const AvdSettings kAvdGingerbreadArm = {9, "arm", "ttyS", false};
const AvdSettings kAvdGingerbreadX86 = {9, "x86", "ttyS", false};
const AvdSettings kAvdIcsArm = {14, "arm", "ttyS", false};
const AvdSettings kAvdIcsX86 = {14, "x86", "ttyS", false};
const AvdSettings kAvdMarshmallowArm = {23, "arm", "ttyAMA", true};
const AvdSettings kAvdMarshmallowArm64 = {23, "arm64", "ttyAMA", true};
const AvdSettings kAvdMarshmallowX86 = {23, "x86", "ttyGF", true};
const AvdSettings kAvdMarshmallowX86_64 = {23, "x86_64", "ttyGF", true};
const AvdSettings kAvdMarshmallowMips = {23, "mips", "ttyS", true};

}  // namespace

TEST(SetupParameters, setupVirtualSerialPorts) {
    static const struct {
        const char* expectedKernelParams;
        const char* expectedQemuParams;
        const AvdSettings* avdSettings;
        bool showKernel;
        bool shellConsole;
    } kData[] = {
            // Gingerbread ARM
            {
                    "android.qemud=ttyS1 console=0",
                    "-serial null -serial android-qemud -serial null",
                    &kAvdGingerbreadArm, false, false,
            },
            {
                    "console=ttyS0 android.qemud=ttyS1",
                    "-serial android-kmsg -serial android-qemud -serial null",
                    &kAvdGingerbreadArm, true, false,
            },
            {
                    "android.qemud=ttyS1 androidboot.console=ttyS2 console=0",
                    "-serial null -serial android-qemud -serial foo-serial",
                    &kAvdGingerbreadArm, false, true,
            },
            {
                    "console=ttyS0 android.qemud=ttyS1 "
                    "androidboot.console=ttyS2",
                    "-serial android-kmsg -serial android-qemud -serial "
                    "foo-serial",
                    &kAvdGingerbreadArm, true, true,
            },

            // Gingerbread X86
            {
                    "android.qemud=ttyS1 console=0",
                    "-serial null -serial android-qemud", &kAvdGingerbreadX86,
                    false, false,
            },
            {
                    "console=ttyS0 android.qemud=ttyS1",
                    "-serial foo-serial -serial android-qemud",
                    &kAvdGingerbreadX86, true, false,
            },
            {
                    "android.qemud=ttyS1 androidboot.console=ttyS0 console=0",
                    "-serial foo-serial -serial android-qemud",
                    &kAvdGingerbreadX86, false, true,
            },
            {
                    "console=ttyS0 android.qemud=ttyS1 "
                    "androidboot.console=ttyS0",
                    "-serial foo-serial -serial android-qemud",
                    &kAvdGingerbreadX86, true, true,
            },

            // Ice Cream Sandwich ARM
            {
                    "android.qemud=1 console=0", "-serial null -serial null",
                    &kAvdIcsArm, false, false,
            },
            {
                    "console=ttyS0 android.qemud=1",
                    "-serial android-kmsg -serial null", &kAvdIcsArm, true,
                    false,
            },
            {
                    "android.qemud=1 androidboot.console=ttyS1 console=0",
                    "-serial null -serial foo-serial", &kAvdIcsArm, false, true,
            },
            {
                    "console=ttyS0 android.qemud=1 androidboot.console=ttyS1",
                    "-serial android-kmsg -serial foo-serial", &kAvdIcsArm,
                    true, true,
            },

            // Ice Cream Sandwich X86
            {
                    "android.qemud=1 console=0", "-serial null -serial null",
                    &kAvdIcsX86, false, false,
            },
            {
                    "console=ttyS0 android.qemud=1",
                    "-serial android-kmsg -serial null", &kAvdIcsX86, true,
                    false,
            },
            {
                    "android.qemud=1 androidboot.console=ttyS1 console=0",
                    "-serial null -serial foo-serial", &kAvdIcsX86, false, true,
            },
            {
                    "console=ttyS0 android.qemud=1 androidboot.console=ttyS1",
                    "-serial android-kmsg -serial foo-serial", &kAvdIcsX86,
                    true, true,
            },

            // Marshmallow ARM
            {
                    "keep_bootcon earlyprintk=ttyAMA0 android.qemud=1 "
                    "console=0",
                    "-serial null", &kAvdMarshmallowArm, false, false,
            },
            {
                    "console=ttyAMA0,38400 keep_bootcon earlyprintk=ttyAMA0 "
                    "android.qemud=1",
                    "-serial foo-serial", &kAvdMarshmallowArm, true, false,
            },
            {
                    "keep_bootcon earlyprintk=ttyAMA0 "
                    "androidboot.console=ttyAMA0 android.qemud=1 console=0",
                    "-serial foo-serial", &kAvdMarshmallowArm, false, true,
            },
            {
                    "console=ttyAMA0,38400 keep_bootcon earlyprintk=ttyAMA0 "
                    "androidboot.console=ttyAMA0 android.qemud=1",
                    "-serial foo-serial", &kAvdMarshmallowArm, true, true,
            },

            // Marshmallow ARM64
            {
                    "keep_bootcon earlyprintk=ttyAMA0 android.qemud=1 "
                    "console=0",
                    "-serial null", &kAvdMarshmallowArm64, false, false,
            },
            {
                    "console=ttyAMA0,38400 keep_bootcon earlyprintk=ttyAMA0 "
                    "android.qemud=1",
                    "-serial foo-serial", &kAvdMarshmallowArm64, true, false,
            },
            {
                    "keep_bootcon earlyprintk=ttyAMA0 "
                    "androidboot.console=ttyAMA0 android.qemud=1 console=0",
                    "-serial foo-serial", &kAvdMarshmallowArm64, false, true,
            },
            {
                    "console=ttyAMA0,38400 keep_bootcon earlyprintk=ttyAMA0 "
                    "androidboot.console=ttyAMA0 android.qemud=1",
                    "-serial foo-serial", &kAvdMarshmallowArm64, true, true,
            },

            // Marshmallow X86
            {
                    "android.qemud=1 console=0", "-serial null",
                    &kAvdMarshmallowX86, false, false,
            },
            {
                    "console=ttyGF0,38400 android.qemud=1",
                    "-serial foo-serial", &kAvdMarshmallowX86, true, false,
            },
            {
                    "androidboot.console=ttyGF0 android.qemud=1 console=0",
                    "-serial foo-serial", &kAvdMarshmallowX86, false, true,
            },
            {
                    "console=ttyGF0,38400 androidboot.console=ttyGF0 "
                    "android.qemud=1",
                    "-serial foo-serial", &kAvdMarshmallowX86, true, true,
            },

            // Marshmallow X86_64
            {
                    "android.qemud=1 console=0", "-serial null",
                    &kAvdMarshmallowX86_64, false, false,
            },
            {
                    "console=ttyGF0,38400 android.qemud=1",
                    "-serial foo-serial", &kAvdMarshmallowX86_64, true, false,
            },
            {
                    "androidboot.console=ttyGF0 android.qemud=1 console=0",
                    "-serial foo-serial", &kAvdMarshmallowX86_64, false, true,
            },
            {
                    "console=ttyGF0,38400 androidboot.console=ttyGF0 "
                    "android.qemud=1",
                    "-serial foo-serial", &kAvdMarshmallowX86_64, true, true,
            },

            // Marshmallow MIPS
            {
                    "android.qemud=1 console=0", "-serial null",
                    &kAvdMarshmallowMips, false, false,
            },
            {
                    "console=ttyS0,38400 android.qemud=1", "-serial foo-serial",
                    &kAvdMarshmallowMips, true, false,
            },
            {
                    "androidboot.console=ttyS0 android.qemud=1 console=0",
                    "-serial foo-serial", &kAvdMarshmallowMips, false, true,
            },
            {
                    "console=ttyS0,38400 androidboot.console=ttyS0 "
                    "android.qemud=1",
                    "-serial foo-serial", &kAvdMarshmallowMips, true, true,
            },
    };
    for (const auto& data : kData) {
        ParameterList kernelParams;
        ParameterList qemuParams;

        android::setupVirtualSerialPorts(
                &kernelParams, &qemuParams,
                data.avdSettings->apiLevel, data.avdSettings->targetArch,
                data.avdSettings->serialPrefix, data.avdSettings->isQemu2,
                data.showKernel, data.shellConsole, "foo-serial");
        std::string logText = android::base::StringFormat(
                "For %s showKernel=%d shellConsole=%d",
                data.avdSettings->toString().c_str(), data.showKernel ? 1 : 0,
                data.shellConsole ? 1 : 0);
        EXPECT_STREQ(data.expectedKernelParams,
                     kernelParams.toString().c_str())
                << logText;

        EXPECT_STREQ(data.expectedQemuParams, qemuParams.toString().c_str())
                << logText;
    }
}

}  // namespace android
