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

#include "android/opengl/EmuglBackendScanner.h"

#include "android/base/Log.h"
#include "android/base/StringFormat.h"
#include "android/base/system/System.h"
#include "android/base/misc/StringUtils.h"

#include "android/utils/path.h"

#include <algorithm>
#include <string>
#include <vector>

namespace android {
namespace opengl {

using android::base::StringFormat;
using android::base::System;

// static
std::vector<std::string> EmuglBackendScanner::scanDir(const char* execDir,
                                                      int programBitness) {
    std::vector<std::string> names;

    if (!execDir || !System::get()->pathExists(execDir)) {
        LOG(ERROR) << "Invalid executable directory: " << execDir;
        return names;
    }
    if (!programBitness) {
        programBitness = System::get()->getProgramBitness();
    }
    const char* subdir = (programBitness == 64) ? "lib64" : "lib";
    std::string subDir = StringFormat("%s/%s/", execDir, subdir);

    std::vector<std::string> entries = System::get()->scanDirEntries(subDir);

    static const char kBackendPrefix[] = "gles_";
    const size_t kBackendPrefixSize = sizeof(kBackendPrefix) - 1U;

    for (size_t n = 0; n < entries.size(); ++n) {
        const std::string& entry = entries[n];
        if (entry.size() <= kBackendPrefixSize ||
            memcmp(entry.c_str(), kBackendPrefix, kBackendPrefixSize)) {
            continue;
        }

        // Check that it is a directory.
        std::string full_dir = StringFormat("%s/%s", subDir.c_str(), entry.c_str());
        if (!System::get()->pathIsDir(full_dir)) {
            continue;
        }
        names.push_back(std::string(entry.c_str() + kBackendPrefixSize));
    }

    // Need to sort the backends in consistent order.
    std::sort(names.begin(), names.end());

    return names;
}

}  // namespace opengl
}  // namespace android
