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

#include "android/base/misc/HttpUtils.h"

#include <gtest/gtest.h>

#include <string.h>

namespace android {
namespace base {

TEST(HttpUtils, httpIsRequestLine) {
    static const struct {
        bool expected;
        const char* line;
    } kData[] = {
        { true, "GET /index.html HTTP/1.0\r\n" },
        { false, "GET THE GRINGO!" },
        { true, "POST /something HTTP/3.14159265359" },
        { false, "POSTAL HTTP/1.1\r\n" },
        { false, "OPTIONS http/1.0\r\n" },
    };
    const size_t kDataSize = sizeof(kData)/sizeof(kData[0]);

    for (size_t n = 0; n < kDataSize; ++n) {
        EXPECT_EQ(kData[n].expected,
                  httpIsRequestLine(
                        kData[n].line,
                        strlen(kData[n].line))) << kData[n].line;
    }
}

}  // namespace base
}  // namespace android
