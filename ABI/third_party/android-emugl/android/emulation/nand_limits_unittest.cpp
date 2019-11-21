// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/emulation/nand_limits.h"

#include <gtest/gtest.h>

#include <stddef.h>

#define ARRAY_SIZE(x)  (sizeof(x)/sizeof(x[0]))

TEST(NandLimits,Parse) {
    static const struct Data {
        const char* args;
        bool result;
        uint64_t read_limit;
        uint64_t write_limit;
        int pid;
        int signal;
    } kData[] = {
        { "pid=128,signal=67,reads=456783", true, 456783ULL, 0ULL, 128, 67 },
        { "signal=65,pid=129,reads=128K", true, 131072ULL, 0ULL, 129, 65 },
        { "reads=6M,pid=130,signal=64", true, 6291456ULL, 0ULL, 130, 64 },
        { "pid=13000,signal=5,reads=12G", true, 12884901888ULL, 0ULL, 13000, 5 },
        { "pid=128,signal=67,writes=456783", true, 0ULL, 456783ULL, 128, 67 },
        { "signal=65,pid=129,writes=128K", true, 0ULL, 131072ULL, 129, 65 },
        { "writes=6M,pid=130,signal=64", true, 0ULL, 6291456ULL, 130, 64 },
        { "pid=13000,signal=5,writes=12G", true, 0ULL, 12884901888ULL, 13000, 5 },
        { "pid=1,signal=2,reads=123,writes=16K", true, 123ULL, 16384ULL, 1, 2 },

        // Empty strings returns an error.
        { "", false, 0ULL, 0ULL, 0, 0 },
        { "     ", false, 0ULL, 0ULL, 0, 0 },
    };
    const size_t kDataSize = ARRAY_SIZE(kData);
    for (size_t n = 0; n < kDataSize; ++n) {
        const Data& expected = kData[n];
        AndroidNandLimit r, w;
        bool result = android_nand_limits_parse(expected.args, &r, &w);
        EXPECT_EQ(expected.result, result) << "#" << n;
        if (result) {
            EXPECT_EQ(expected.pid, r.pid) << "#" << n;
            EXPECT_EQ(expected.signal, r.signal) << "#" << n;
            EXPECT_EQ(expected.read_limit, r.limit) << "#" << n;
            EXPECT_EQ(expected.pid, w.pid) << "#" << n;
            EXPECT_EQ(expected.signal, w.signal) << "#" << n;
            EXPECT_EQ(expected.write_limit, w.limit) << "#" << n;
        }
    }
}
