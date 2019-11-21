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

#include "android/base/Log.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/base/misc/StringUtils.h"
#include "android/kernel/kernel_utils.h"
#include "android/kernel/kernel_utils_testing.h"
#include "android/uncompress.h"
#include "android/utils/file_data.h"
#include "android/utils/path.h"
#include "android/utils/string.h"

#include <vector>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define DEBUG_KERNEL  0

#define KERNEL_LOG     LOG_IF(INFO, DEBUG_KERNEL)
#define KERNEL_PLOG    PLOG_IF(INFO, DEBUG_KERNEL)
#define KERNEL_ERROR   LOG_IF(ERROR, DEBUG_KERNEL)
#define KERNEL_PERROR  PLOG_IF(ERROR, DEBUG_KERNEL)

namespace {

const char kLinuxVersionStringPrefix[] = "Linux version ";
const size_t kLinuxVersionStringPrefixLen =
        sizeof(kLinuxVersionStringPrefix) - 1U;

}  // namespace

bool android_parseLinuxVersionString(const char* versionString,
                                     KernelVersion* kernelVersion) {
    if (strncmp(versionString, kLinuxVersionStringPrefix,
            kLinuxVersionStringPrefixLen)) {
        KERNEL_ERROR << "unsupported kernel version string:" << versionString;
        return false;
    }
    // skip past the prefix to the version number
    versionString += kLinuxVersionStringPrefixLen;

    uint32_t temp = 0;
    for (int i = 0; i < 3; i++) {
        // skip '.' delimeters
        while (i > 0 && *versionString == '.') {
            versionString++;
        }

        char* end;
        unsigned long number = ::strtoul(versionString, &end, 10);
        if (end == versionString || number > 0xff) {
            KERNEL_ERROR << "unsupported kernel version string:"
                         << versionString;
            return false;
        }
        temp <<= 8;
        temp |= number;
        versionString = end;
    }
    *kernelVersion = (KernelVersion)temp;

    KERNEL_LOG << android::base::LogString("Kernel version hex 0x%06x", temp);
    return true;
}

const char* android_kernelSerialDevicePrefix(KernelVersion kernelVersion) {
    if (kernelVersion >= KERNEL_VERSION_3_10_0) {
        return "ttyGF";
    }
    return "ttyS";
}

bool android_imageProbeKernelVersionString(const uint8_t* kernelFileData,
                                           size_t kernelFileSize,
                                           char* dst/*[dstLen]*/,
                                           size_t dstLen) {
    std::vector<uint8_t> uncompressed;

    const uint8_t* uncompressedKernel = NULL;
    size_t uncompressedKernelLen = 0;

    const char kElfHeader[] = { 0x7f, 'E', 'L', 'F' };

    if (kernelFileSize < sizeof(kElfHeader)) {
        KERNEL_ERROR << "Kernel image too short";
        return false;
    }

    const char* versionStringStart = NULL;

    if (0 == memcmp(kElfHeader, kernelFileData, sizeof(kElfHeader))) {
        // this is an uncompressed ELF file (probably mips)
        uncompressedKernel = kernelFileData;
        uncompressedKernelLen = kernelFileSize;
    }
    else {
        // handle compressed kernels here
        const uint8_t kGZipMagic[4] = { 0x1f, 0x8b, 0x08, 0x00 };
        const uint8_t* compressedKernel = (const uint8_t*)memmem(kernelFileData,
                                                                 kernelFileSize,
                                                                 kGZipMagic,
                                                                 sizeof(kGZipMagic));
        if (!compressedKernel) {
            KERNEL_ERROR << "Could not find gzip header in kernel file!";
            return false;
        }

        // Special case: certain images, like the ARM64 one, contain a GZip
        // header _after_ the actual Linux version string. So first try to
        // see if there is something before the header.
        versionStringStart = (const char*)memmem(
                kernelFileData,
                (compressedKernel - kernelFileData),
                kLinuxVersionStringPrefix,
                kLinuxVersionStringPrefixLen);

        if (!versionStringStart) {
            size_t compressedKernelLen = kernelFileSize -
                (compressedKernel - kernelFileData);

            // inflate ratios for all prebuilt kernels on 2014-07-14 is 1.9:1 ~
            // 3.43:1 not sure how big the uncompressed size is, so make an
            // absurdly large buffer
            uncompressedKernelLen = compressedKernelLen * 10;
            uncompressed.resize(uncompressedKernelLen);
            uncompressedKernel = uncompressed.data();

            bool zOk = uncompress_gzipStream(uncompressed.data(),
                                            &uncompressedKernelLen,
                                            compressedKernel,
                                            compressedKernelLen);
            if (!zOk) {
                KERNEL_ERROR << "Kernel decompression error";
                // it may have been partially decompressed, so we're going to
                // try to find the version string anyway
            }
        }
    }

    if (!versionStringStart) {
        versionStringStart = (const char*)memmem(
                uncompressedKernel,
                uncompressedKernelLen,
                kLinuxVersionStringPrefix,
                kLinuxVersionStringPrefixLen);

        if (!versionStringStart) {
            KERNEL_ERROR << "Could not find 'Linux version ' in kernel!";
            return false;
        }
    }

    strlcpy(dst, versionStringStart, dstLen);

    return true;
}

bool android_pathProbeKernelVersionString(const char* kernelPath,
                                          char* dst/*[dstLen]*/,
                                          size_t dstLen) {
    FileData kernelFileData;
    if (fileData_initFromFile(&kernelFileData, kernelPath) < 0) {
        KERNEL_ERROR << "Could not open kernel file!";
        return false;
    }

    const auto kernelFileDeleter =
            android::base::makeCustomScopedPtr(&kernelFileData, fileData_done);

    return android_imageProbeKernelVersionString(kernelFileData.data,
                                                 kernelFileData.size,
                                                 dst,
                                                 dstLen);
}
