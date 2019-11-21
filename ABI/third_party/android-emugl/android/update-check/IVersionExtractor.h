// Copyright 2015-2016 The Android Open Source Project
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

#include "android/base/Version.h"
#include "android/base/StringView.h"

#include "android/metrics/StudioConfig.h"

#include <map>

namespace android {
namespace update_check {

// Interface IVersionExtractor - extract all emulator versions
// with their update channels from the xml manifest
class IVersionExtractor {
public:
    using StringView = android::base::StringView;
    using UpdateChannel = android::studio::UpdateChannel;
    using Version = android::base::Version;

    using Versions = std::map<UpdateChannel, Version>;

    // Returns the list of the versions by channel extracted from the |data|,
    // or empty collection if the data format is incorrect
    virtual Versions extractVersions(StringView data) const = 0;

    // Returns the current application version
    virtual Version getCurrentVersion() const = 0;

    virtual ~IVersionExtractor() = default;
};

}  // namespace update_check
}  // namespace android
