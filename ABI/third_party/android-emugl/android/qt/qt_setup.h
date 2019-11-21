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

#include <stdbool.h>

ANDROID_BEGIN_HEADER

// Setup environment variables to run the Qt-based UI programs.
// This function should be called from the top-level launcher and will
// modify the library search path when needed.
// Return true in case of success, false if there is no corresponding
// Qt library directory.
// |bitness| - target program bittness, 32/64 or 0 for autodetection
// |emulatorDir| - the directory where 'emulator' is located; if not given,
// it will be inferred based on the location of the calling binary of this
// function
bool androidQtSetupEnv(int bitness, const char* emulatorDir = nullptr);

ANDROID_END_HEADER
