// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#include "android/emulation/ComponentVersion.h"

#include "android/base/files/PathUtils.h"

#include <cstdio>
#include <fstream>

namespace android {

using android::base::Version;
using android::base::PathUtils;
using android::base::StringView;

static Version parseVersionFromSourceProperties(
        const std::string& propertiesPath) {
    Version version = Version::invalid();
    std::ifstream propertiesFile(propertiesPath.c_str());
    if (propertiesFile.is_open()) {
        // Find the line containing "Pkg.Revision".
        std::string line;
        char verStr[100];
        while (std::getline(propertiesFile, line)) {
            if (sscanf(line.c_str(), " Pkg.Revision = %s", verStr) == 1) {
                version = Version(verStr);
                break;
            }
        }
    }
    return version;
}

static std::string getComponentPath(android::base::StringView sdkRootDirectory,
                                    SdkComponentType type) {
    switch (type) {
        case SdkComponentType::PlatformTools:
            return PathUtils::join(sdkRootDirectory, "platform-tools");
        case SdkComponentType::Tools:
            return PathUtils::join(sdkRootDirectory, "tools");
        default:
            return "";
    }
}

android::base::Version getCurrentSdkVersion(
        android::base::StringView sdkRootDirectory,
        SdkComponentType type) {
    Version version = Version::invalid();
    if (!sdkRootDirectory.empty()) {
        std::string componentPath = getComponentPath(sdkRootDirectory, type);
        if (!componentPath.empty()) {
            version = parseVersionFromSourceProperties(
                    PathUtils::join(componentPath, "source.properties"));
        }
    }
    return version;
}

}  // namespace android