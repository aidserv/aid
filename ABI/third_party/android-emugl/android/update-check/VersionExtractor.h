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

#include "android/update-check/IVersionExtractor.h"

namespace android {
namespace update_check {

class VersionExtractor : public IVersionExtractor {
public:
    static const char* const kXmlNamespace;

    virtual Versions extractVersions(StringView data) const override;

    virtual Version getCurrentVersion() const override;
};

}  // namespace update_check
}  // namespace android
