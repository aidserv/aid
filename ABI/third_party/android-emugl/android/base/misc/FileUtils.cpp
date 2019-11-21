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
#include "android/utils/eintr_wrapper.h"

#include <assert.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace android {

bool readFileIntoString(int fd, std::string* file_contents) {
    off_t size = lseek(fd, 0, SEEK_END);
    if (size == (off_t)-1) {
        return false;
    }
    off_t err = lseek(fd, 0, SEEK_SET);
    if (err == (off_t)-1) {
        return false;
    }

    std::vector<char> buf(size);
    ssize_t result = HANDLE_EINTR(read(fd, &buf[0], size));
    if (result != size) {
        return false;
    }
    file_contents->assign(&buf[0], result);
    return true;
}

bool writeStringToFile(int fd, const std::string& file_contents) {
    ssize_t result = HANDLE_EINTR(
            write(fd, file_contents.c_str(), file_contents.size()));
    if (result != (ssize_t)file_contents.size()) {
        return false;
    }
    return true;
}

}  // namespace android
