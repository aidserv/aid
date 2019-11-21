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

#pragma once

#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

#ifndef __linux__
// Make sure these are defined and don't change anything if used.
enum {
    SOCK_CLOEXEC = 0,
#ifndef __APPLE__
    O_CLOEXEC = 0
#endif
};
#endif  // !__linux__

// A C wrapper for android::base::fdSetCloexec().
void fd_set_cloexec(int fd);

ANDROID_END_HEADER
