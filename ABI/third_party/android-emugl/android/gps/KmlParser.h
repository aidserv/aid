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

#include "android/gps/GpsFix.h"

#include <string>

class KmlParser {
public:
    // Parses a given .kml file at |filePath| and extracts all contained GPS
    // fixes into |*fixes|.
    // Returns true on success, false otherwise. if false is returned, |*error|
    // is set to a message describing the error.
    static bool parseFile(const char * filePath, GpsFixArray * fixes, std::string * error);
};
