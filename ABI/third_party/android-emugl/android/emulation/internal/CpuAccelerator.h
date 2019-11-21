// Copyright (C) 2014 The Android Open Source Project
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

#include "android/cpu_accelerator.h"
#include <string>
#include <stdlib.h>

namespace android {

#ifdef __APPLE__

// This function takes a string in the format "VERSION=1.2.3" and returns an
// integer that represents the dotted version number.  Invalid strings are
// converted to -1.
// Any characters before "VERSION=" are ignored.
// Non-numeric characters at the end are ignored.
//
// Examples:
//   "VERSION=1.2.4" -> 0x01020004
//   "VERSION=1.2"   -> 0x01020000
//   "1.2.4"         -> -1
//   "asfd"          -> -1

int32_t cpuAcceleratorParseVersionScript(
        const std::string& version_script);

// This function searches one or more kernel extension directories for the
// specified version file.
// The version file is read and parsed if found
// cpuAcceleratorParseVersionScript
// returns 0 - HAXM not installed
// returns -1 - HAXM corrupt or too old
// returns 0x01020004 for version version number e.g. "1.2.4"
int32_t cpuAcceleratorGetHaxVersion(const char* kext_dir[],
                                    const size_t kext_dir_count,
                                    const char* version_file);

#endif

}  // namespace android
