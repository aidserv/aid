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

#pragma once

#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

// This is a generic wrapper over the exec() system call. It tries to hide
// the OS-depended differences from the user.
//  On Posix it's just a raw execv() with passed arguments
//  On Windows this actually spawns a child process and waits for it to finish
//   this is needed to workaround poor console implementation which can't
//   really support multiple processes on the same console well

int safe_execv(const char* path, char* const* argv);

ANDROID_END_HEADER
