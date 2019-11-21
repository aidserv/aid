// Copyright 2015 The Android Open Source Project
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

#include "android/base/Version.h"

#include <functional>
#include <type_traits>

namespace android {
namespace studio {

// The set of existing update channels, ordered from most to least stable.
enum class UpdateChannel { Stable, Beta, Dev, Canary, Unknown };

// Extract the Android Studio version from directory name |dirName|.
// dirName is expected to match the following pattern
//            [.]AndroidStudio[Preview]X.Y
// where X,Y represent a major/minor revision number.
// If |dirName| is not formatted following the AndroidStudio
// preferences/configuration directory name pattern, an invalid
// Version will be returned.
//
android::base::Version extractAndroidStudioVersion(const char* const dirName);

// Scan path |scanPath| for AndroidStudio preferences/configuration
// directries and return the path to the latest one, or NULL
// if none can be found. Returned value is a String
//
std::string latestAndroidStudioDir(const std::string& scanPath);

// Construct and return the complete path to Android Studio XML
// preferences |filename| under path |studioPath|.
// Returned value is a String
std::string pathToStudioXML(const std::string& studioPath,
                            const std::string& filename);

// Returns the currently selected update channel for the latest version
// of Android Studio
UpdateChannel updateChannel();

// Checks if the user has opted in for metrics reporting.
bool getUserMetricsOptIn();

// This function returns a string that describes the Android
// Studio installation ID. If this installation ID cannot be
// retrieved, a random string following the Android Studio
// pattern of installation IDs
// (00000000-0000-0000-0000-000000000000) will be returned.
const std::string& getInstallationId();

}  // namespace studio
}  // namespace android

// make sure the UpdateChannel enum is hashable
namespace std {

template <>
struct hash<android::studio::UpdateChannel> {
    size_t operator()(android::studio::UpdateChannel ch) const {
        using IntType =
                std::underlying_type<android::studio::UpdateChannel>::type;
        return std::hash<IntType>()(static_cast<IntType>(ch));
    }
};

}  // namespace std
