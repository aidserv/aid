// Copyright (C) 2016 The Android Open Source Project
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

namespace android {

// Reads from |fd| into |file_contents|
// Returns false if something went wrong
bool readFileIntoString(int fd, std::string* file_contents);

// Writes |file_contents| to |fd|
// Returns false if something went wrong
bool writeStringToFile(int fd, const std::string& file_contents);

}  // namespace android
