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

#include "android/base/Version.h"
#include "android/base/StringFormat.h"

#include <sstream>
#include <assert.h>

namespace android {
namespace base {

static constexpr StringView kInvalidVersion = "invalid";

static bool isEof(std::istream& in) {
    return in.peek() == std::char_traits<char>::eof();
}

Version::Version(StringView ver) : mData() {
    std::istringstream in(ver);
    in >> std::noskipws;

    // read the main part, major.minor.micro
    in >> std::get<kMajor>(mData);
    if (!in) {
        *this = invalid();
        return;
    }

    static const char delimiters[kComponentCount - 1] = {'.', '.', '-'};
    for (int comp = kMinor; comp <= kBuild && !isEof(in); ++comp) {
        char c;
        in >> c >> component(static_cast<Component>(comp));
        if (c != delimiters[comp - 1] || !in) {
            *this = invalid();
            return;
        }
    }

    // make sure the stream is consumed to the end
    if (!isEof(in)) {
        *this = invalid();
    }
}

std::string Version::toString() const {
    if (!isValid()) {
        return kInvalidVersion;
    }

    std::string res = StringFormat("%u.%u.%u", component<kMajor>(),
                                   component<kMinor>(), component<kMicro>());
    if (component<kBuild>() != kNone && component<kBuild>() != 0) {
        StringAppendFormat(&res, "-%u", component<kBuild>());
    }
    return res;
}

Version::ComponentType& Version::component(Version::Component c) {
    // this has to be a switch: tuple isn't a container, so it doesn't
    // provide runtime indexing
    switch (c) {
        case kMajor:
            return std::get<kMajor>(mData);
        case kMinor:
            return std::get<kMinor>(mData);
        case kMicro:
            return std::get<kMicro>(mData);
        case kBuild:
            return std::get<kBuild>(mData);
    }

    assert(false);
    static ComponentType none = kNone;
    return none;
}

}  // namespace android
}  // namespace base
