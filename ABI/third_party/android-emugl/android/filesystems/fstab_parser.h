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

// Parse the Linux fstab file at |fstabData| of |fstabSize| bytes
// and extract the format of the partition named |partitionName|.
// On success, return true and sets |*outFormat| to a heap-allocated
// string that must be freed by the caller. On failure, return false.
bool android_parseFstabPartitionFormat(const char* fstabData,
                                       size_t fstabDize,
                                       const char* partitionName,
                                       char** outFormat);

ANDROID_END_HEADER
