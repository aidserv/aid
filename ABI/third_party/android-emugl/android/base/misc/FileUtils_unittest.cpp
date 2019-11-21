// Copyright (C) 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/misc/FileUtils.h"

#include "android/base/files/ScopedFd.h"
#include "android/utils/eintr_wrapper.h"
#include "android/utils/tempfile.h"

#include <fcntl.h>
#include <gtest/gtest.h>
#include <limits>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>

using android::base::ScopedFd;

namespace android {

TEST(FileUtils, stringToFile) {
    const char test_pattern[] = "test pattern";
    TempFile* tf = tempfile_create();

    ScopedFd fd(HANDLE_EINTR(open(tempfile_path(tf), O_RDWR, 0600)));
    EXPECT_NE(-1, fd.get());

    EXPECT_FALSE(writeStringToFile(-1, test_pattern));
    EXPECT_TRUE(writeStringToFile(fd.get(), test_pattern));

    std::string dummy_buffer("dummy");
    EXPECT_FALSE(readFileIntoString(-1, &dummy_buffer));
    EXPECT_STREQ("dummy", dummy_buffer.c_str());

    std::string read_buffer;
    EXPECT_TRUE(readFileIntoString(fd.get(), &read_buffer));
    EXPECT_STREQ(test_pattern, read_buffer.c_str());

    fd.close();

    tempfile_close(tf);
}

}  // namespace android
