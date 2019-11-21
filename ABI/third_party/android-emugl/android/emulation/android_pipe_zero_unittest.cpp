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

#include "android/emulation/testing/TestAndroidPipeDevice.h"

#include <gtest/gtest.h>

#include <memory>
#include <string>

#define ARRAY_SIZE(x)  (sizeof(x)/sizeof(x[0]))

using Guest = android::TestAndroidPipeDevice::Guest;

// A TestAndroidPipeDevice that provides the 'zero' service.
extern "C" void android_pipe_add_type_zero(void);

class ZeroPipeDevice : public android::TestAndroidPipeDevice {
public:
    ZeroPipeDevice() {
        android_pipe_add_type_zero();
    }
};

TEST(AndroidPipe,ZeroPipeWrites) {
    ZeroPipeDevice dev;
    std::unique_ptr<Guest> guest(Guest::create());
    EXPECT_EQ(0, guest->connect("zero"));
    EXPECT_EQ((unsigned)(PIPE_POLL_IN|PIPE_POLL_OUT), guest->poll());

    static const size_t kSizes[] = {
        0, 100, 128, 256, 512, 1000, 2048, 8192, 655356,
    };
    for (size_t n = 0; n < ARRAY_SIZE(kSizes); n++) {
        std::string buffer('x', kSizes[n]);
        EXPECT_EQ((unsigned)(PIPE_POLL_IN|PIPE_POLL_OUT), guest->poll());
        EXPECT_EQ((ssize_t)buffer.size(),
                  guest->write(buffer.c_str(), buffer.size()));
    }
}

TEST(AndroidPipe,ZeroPipeReads) {
    ZeroPipeDevice dev;
    std::unique_ptr<Guest> guest(Guest::create());
    EXPECT_EQ(0, guest->connect("zero"));
    EXPECT_EQ((unsigned)(PIPE_POLL_IN|PIPE_POLL_OUT), guest->poll());

    static const size_t kSizes[] = {
        0, 100, 128, 256, 512, 1000, 2048, 8192, 655356,
    };
    for (size_t n = 0; n < ARRAY_SIZE(kSizes); n++) {
        std::string buffer('x', kSizes[n]);
        EXPECT_EQ((unsigned)(PIPE_POLL_IN|PIPE_POLL_OUT), guest->poll());
        EXPECT_EQ((ssize_t)buffer.size(),
                  guest->read(&buffer[0], buffer.size()));
        for (size_t i = 0; i < buffer.size(); ++i) {
            EXPECT_EQ('\0', buffer[i]) << "# "<< i << " / " << kSizes[n];
        }
    }
}
