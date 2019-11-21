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

// This is the only knob you need to touch to perform a daily
// emulator updates check. This function performs a asyncronous update check
// and prints a message to the user if there's a newer version available.
// It will skip the check if it has already been done on the same day
// |coreVersion| - core-specific version, e.g. "qemu2 2.2.0"
extern void android_checkForUpdates(const char* coreVersion);

ANDROID_END_HEADER
