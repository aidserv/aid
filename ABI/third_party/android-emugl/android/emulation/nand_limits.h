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
#include <stdint.h>

ANDROID_BEGIN_HEADER

// A structure describing option limit threshold to be used at runtime
// during NAND emulation. This is used to monitor NAND activity by sending
// a |signal| to an external process |pid| once |limit| bytes have been
// read (or written). |counter| is just a runtime counter that must be
// initialized to 0.
typedef struct {
    uint64_t limit;
    uint64_t counter;
    int pid;
    int signal;
} AndroidNandLimit;

// Static initializer for an AndroidNandLimit structure.
#define ANDROID_NAND_LIMIT_INIT \
    { 0ULL, 0ULL, -1, -1 }

// Parse a -nand-limit=<args> command-line option, and write the results
// to |*read_limit| and |*write_limit|. |args| is the argument string,
// which must be a comma-delimited list of <key>=<value> assigments,
// with the following supported keys:
//
//   pid=<number>
//      Required, process ID where to send a signal when either one of the
//      limits is reached.
//
//   signal=<number>
//      Required, signal number to send to <pid> when eithe one of the limits
//      is reached.
//
//   reads=<size>
//   writes=<size>
//      Optional read and write limit thresholds. <size> can be a number of
//      bytes, of a decimal number followed by a size suffix like K, M or G
//      for kilobytes (KiB), megabytes (MiB) and gigabytes (GiB), respectively.
//
// Return true on success, or false on failure.
bool android_nand_limits_parse(const char* args,
                               AndroidNandLimit* read_limit,
                               AndroidNandLimit* write_limit);

// Call this to update the |counter| of an AndroidNandLimit instance |limit|.
// If it reaches the limit threshold, a signal will be sent to a registered
// pid, unless none was configured by android_nand_limits_parse.
void android_nand_limit_update(AndroidNandLimit* limit, uint32_t count);

ANDROID_END_HEADER
