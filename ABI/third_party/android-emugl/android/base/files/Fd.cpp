// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/files/Fd.h"

#ifndef _WIN32
#   include <unistd.h>
#   include <fcntl.h>
#endif  // !_WIN32

namespace android {
namespace base {

void fdSetCloexec(int fd) {
#ifndef _WIN32
    int f = ::fcntl(fd, F_GETFD);
    ::fcntl(fd, F_SETFD, f | FD_CLOEXEC);
#endif  // !_WIN32
}

}  // namespace base
}  // namespace android
