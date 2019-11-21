// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/metrics/MetricsPaths.h"

#include "android/base/files/PathUtils.h"
#include "android/emulation/ConfigDirs.h"

using android::base::PathUtils;
using android::ConfigDirs;

namespace android {
namespace metrics {

std::string getSettingsFilePath() {
    return PathUtils::join(
                ConfigDirs::getUserDirectory(), "analytics.settings");
}

std::string getSpoolDirectory() {
    return PathUtils::join(
                ConfigDirs::getUserDirectory(), "metrics", "spool");
}

}  // namespace metrics
}  // namespace android
