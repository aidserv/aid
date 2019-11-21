// Copyright (C) 2015 The Android Open Source Project
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

#include <string>
#include <vector>

// A struct representing a location on a map
struct GpsFix {
    std::string name;
    std::string description;
    float latitude = 0.0;
    float longitude = 0.0;
    float elevation = 0.0;
    time_t time = 0;

    bool operator <(const GpsFix &other) const {
        return time < other.time;
    }
};

typedef std::vector<GpsFix> GpsFixArray;
