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

#include "android/emulation/AdbDebugPipe.h"

#include "android/base/StringView.h"
#include "android/base/testing/TestMemoryOutputStream.h"
#include "android/emulation/testing/TestAndroidPipeDevice.h"

#include <gtest/gtest.h>

namespace android {
namespace emulation {

using android::base::StringView;

TEST(AdbDebugPipe, noOutput) {
    TestAndroidPipeDevice testDevice;
    AndroidPipe::Service::add(new AdbDebugPipe::Service(nullptr));

    auto guest = TestAndroidPipeDevice::Guest::create();
    EXPECT_EQ(0, guest->connect("qemud:adb-debug"));
    constexpr StringView kMessage = "Hello debug world!";
    EXPECT_EQ(static_cast<ssize_t>(kMessage.size()),
              guest->write(kMessage.c_str(), kMessage.size()));
}

TEST(AdbDebugPipe, withOutput) {
    TestAndroidPipeDevice testDevice;
    auto outStream = new android::base::TestMemoryOutputStream();
    AndroidPipe::Service::add(new AdbDebugPipe::Service(outStream));

    auto guest = TestAndroidPipeDevice::Guest::create();
    EXPECT_EQ(0, guest->connect("qemud:adb-debug"));
    constexpr StringView kMessage = "Hello debug world!";
    EXPECT_EQ(static_cast<ssize_t>(kMessage.size()),
              guest->write(kMessage.c_str(), kMessage.size()));

    EXPECT_EQ(kMessage, outStream->view());
}

}  // namespace emulation
}  // namespace android
