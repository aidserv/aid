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

#pragma once

#include "android/utils/compiler.h"

#include <stddef.h>


ANDROID_BEGIN_HEADER

// Extract the content of a given file from a ramdisk image.
// |ramdisk_path| is the path to the ramdisk.img file.
// |file_path| is the path of the file within the ramdisk.
// On success, returns true and sets |*out| to point to a heap allocated
// block containing the extracted content, of size |*out_size| bytes.
// On failure, return false.
bool android_extractRamdiskFile(const char* ramdisk_path,
                                const char* file_path,
                                char** out,
                                size_t* out_size);

ANDROID_END_HEADER
