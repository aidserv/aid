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

// A TestAndroidPipeDevice that provides the 'pingpong' service.
extern "C" void android_pipe_add_type_pingpong(void);

class PingPongPipeDevice : public android::TestAndroidPipeDevice {
public:
    PingPongPipeDevice() {
        android_pipe_add_type_pingpong();
    }
};

TEST(AndroidPipe, PingPongPipeWritesAndReadsOfTheSameSize) {
    PingPongPipeDevice dev;
    std::unique_ptr<Guest> guest(Guest::create());
    EXPECT_EQ(0, guest->connect("pingpong"));
    EXPECT_EQ(PIPE_POLL_OUT, guest->poll());

    static const size_t kSizes[] = {
        0, 100, 128, 256, 512, 1000, 2048, 8192, 655356,
    };
    for (size_t n = 0; n < ARRAY_SIZE(kSizes); n++) {
        size_t size = kSizes[n];

        // Allocate and fill buffer with simple pattern.
        std::string buffer(size, 'x');
        for (size_t i = 0u; i < size; ++i) {
            buffer[i] = (char)(i + size);
        }

        // Send buffer to service.
        EXPECT_EQ(PIPE_POLL_OUT, guest->poll());
        EXPECT_EQ(static_cast<ssize_t>(size),
                  guest->write(buffer.c_str(), size));
        if (size > 0) {
            EXPECT_EQ((unsigned)(PIPE_POLL_IN|PIPE_POLL_OUT), guest->poll());
            // Erase buffer, then receive data from service.
            memset(&buffer[0], '\xff', size);
            EXPECT_EQ(static_cast<ssize_t>(size),
                      guest->read(&buffer[0], size));
            EXPECT_EQ((unsigned)PIPE_POLL_OUT, guest->poll());

            // Check returned pattern
            for (size_t i = 0; i < size; ++i) {
                EXPECT_EQ((char)(i + size), buffer[i]) << "# " << i << " / " << size;
            }
        } else {
            EXPECT_EQ((unsigned)PIPE_POLL_OUT, guest->poll());
            EXPECT_GT(0, guest->read(&buffer[0], size));
        }
    }
}

TEST(AndroidPipe, PingPongPipeLargeWriteWithSmallReads) {
   PingPongPipeDevice dev;
    std::unique_ptr<Guest> guest(Guest::create());
    EXPECT_EQ(0, guest->connect("pingpong"));
    EXPECT_EQ(PIPE_POLL_OUT, guest->poll());

    const size_t size = 100000;
    std::string buffer(size, 'x');
    for (size_t i = 0; i < size; ++i) {
        buffer[i] = (char)i;
    }

    EXPECT_EQ(static_cast<ssize_t>(size), guest->write(buffer.c_str(), size));

    for (size_t i = 0; i < size; i++) {
        char tmp[1];
        EXPECT_EQ((unsigned)(PIPE_POLL_IN|PIPE_POLL_OUT), guest->poll());
        EXPECT_EQ(1, guest->read(tmp, 1));
        EXPECT_EQ((char)i, tmp[0]) << "# " << i << " expected " << (char)i;
    }
    EXPECT_EQ((unsigned)PIPE_POLL_OUT, guest->poll());
}

TEST(AndroidPipe, PingPongSmallWritesAndLargeRead) {
   PingPongPipeDevice dev;
    std::unique_ptr<Guest> guest(Guest::create());
    EXPECT_EQ(0, guest->connect("pingpong"));
    EXPECT_EQ((unsigned)PIPE_POLL_OUT, guest->poll());

    const size_t size = 100000;
    std::string buffer(size, 'x');
    for (size_t i = 0; i < size; ++i) {
        buffer[i] = (char)i;
    }

    for (size_t i = 0; i < size; i++) {
        EXPECT_EQ(1, guest->write(&buffer[i], 1)) << "# " << i << " / " << size;
        EXPECT_EQ((unsigned)(PIPE_POLL_IN|PIPE_POLL_OUT), guest->poll());
    }

    memset(&buffer[0], '\xff', size);

    EXPECT_EQ((ssize_t)size, guest->read(&buffer[0], size));
    EXPECT_EQ((unsigned)PIPE_POLL_OUT, guest->poll());

    for (size_t i = 0; i < size; i++) {
        EXPECT_EQ((char)i, buffer[i]) << "# " << i << " / " << size;
    }
    EXPECT_EQ((unsigned)PIPE_POLL_OUT, guest->poll());
}
