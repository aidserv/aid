/* Copyright (C) 2015 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

#pragma once

#if !defined(_WIN32) && !defined(_WIN64)
#error "Only compile this file when targetting Windows!"
#endif

#include <stdint.h>

namespace android {

class WindowsInstaller
{
public:
    /*
     * GetVersion
     *
     * Searches
     * "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\
     *    Installer\UserData\S-1-5-18\Products"
     * For the first product with a "DisplayName" string that matches
     * |productDisplayName|
     * Returns the "Version" DWORD value of that product if found
     * returns kNotInstalled if |productDisplayName| was not found in any
     * DisplayName values
     * returns kUnknown if an error occurred
     */
    enum {
        // Legal version numbers are positive.  These constants must be
        // non-positive so (getVersion < min_version) will be true regardless of
        // whether the package status is not-installed, obsolete or unknown
        kNotInstalled = 0,
        kUnknown = -1,  // some error occurred
    };
    static int32_t getVersion(const char* productDisplayName);
};

}  // namespace android
