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

// Probe the kernel image at |kernelPath| and copy the corresponding
// 'Linux version ' string into the |dst| buffer.  On success, returns true
// and copies up to |dstLen-1| characters into dst.  dst will always be NUL
// terminated if |dstLen| >= 1
//
// On failure (e.g. if the file doesn't exist or cannot be probed), return
// false and doesn't touch |dst| buffer.
bool android_imageProbeKernelVersionString(const uint8_t* kernelFileData,
                                           size_t kernelFileSize,
                                           char* dst,
                                           size_t dstLen);

