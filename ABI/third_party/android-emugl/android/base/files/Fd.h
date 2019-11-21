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

// Include this for the definition of enums missing on different platforms.
#include "android/utils/fd.h"

namespace android {
namespace base {

// Make a file descriptor "close-on-exec" to make sure it won't get inherided
// by a fork-execed child.
void fdSetCloexec(int fd);

}  // namespace base
}  // namespace android
