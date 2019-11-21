// Copyright (C) 2015 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "android/utils/compiler.h"
#include <stdint.h>

ANDROID_BEGIN_HEADER

// Resize the partition in |partitionPath| to be newSize.
//
// |partitionPath| should hold the full, null-terminated path to the
// desired partition.
// |newByteSize| should be the desired new size of the partition in bytes,
// no smaller than 128 MiB and no greater than 16 TiB. If the |newByteSize|
// is below the minimum allowed for the specified partition, resize2fs will
// still fail and the error code will be returned to the user
//
// Returns:
//     0 - indicating the resize was successful
//    -1 - indicating that formatting the arguments failed
//    -2 - indicating a system call went wrong
//    Otherwise the exit code of the resize2fs process is returned.
int resizeExt4Partition(const char* partitionPath, int64_t newByteSize);

// Returns true if |byteSize| is a valid ext4 partition size; i.e. within the
// range of 128 MiB and 16 TiB inclusive, false otherwise.
//
// ext4 partitions have a theoretical limit of 1 EiB, although on 32-bit
// systems the partition limit is reduced to 16 TiB due to the data range
// of 32-bits, so enforce 16 TiB to minimize differences between systems of
// different bitness.  The minimum size of an ext4 partition is 128 MiB.
bool checkExt4PartitionSize(int64_t byteSize);

ANDROID_END_HEADER
