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

#include <windows.h>
#include <memory>
#include <string>

#include "android/base/files/ScopedRegKey.h"
#include "android/base/StringFormat.h"
#include "android/base/system/Win32UnicodeString.h"
#include "android/base/system/Win32Utils.h"
#include "android/windows_installer.h"

namespace android {

using base::StringAppendFormat;
using base::ScopedRegKey;
using base::Win32UnicodeString;
using base::Win32Utils;

int32_t WindowsInstaller::getVersion(const char* productDisplayName) {
    DWORD cSubKeys = 0;  // number of subkeys
    REGSAM samDesired = KEY_READ;
    DWORD dwGetValueFlags = 0;

#if !_WIN64
    // 32 bit binary
    BOOL bIsWow64 = FALSE;
    if (IsWow64Process(GetCurrentProcess(), &bIsWow64)) {
        if (bIsWow64) {
            // this is a 32 bit binary running on a 64 bit host, check the 64
            // bit registry
            samDesired |= KEY_WOW64_64KEY;
            const int RRF_SUBKEY_WOW6464KEY = 0x00010000;
            dwGetValueFlags |= RRF_SUBKEY_WOW6464KEY;
        }
    }
#endif

    HKEY hkey = 0;
    const char* registry_path =
            "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\"
            "Installer\\UserData\\S-1-5-18\\Products";
    LONG result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, registry_path, 0, samDesired,
                               &hkey);
    if (result != ERROR_SUCCESS) {
        std::string error_string = Win32Utils::getErrorString(result);
        printf("RegOpenKeyEx failed %li %s\n", result, error_string.c_str());
        return kUnknown;
    }
    ScopedRegKey hProductsKey(hkey);

    result = RegQueryInfoKeyW(hProductsKey.get(), nullptr, nullptr, nullptr,
                              &cSubKeys, nullptr, nullptr, nullptr, nullptr,
                              nullptr, nullptr, nullptr);
    if (result != ERROR_SUCCESS) {
        std::string error_string = Win32Utils::getErrorString(result);
        printf("RegQueryInfoKeyW failed %li %s\n", result,
               error_string.c_str());
        return kUnknown;
    }

    for (DWORD i = 0; i < cSubKeys; i++) {
        const size_t kMaxKeyLength = 256;
        char product_guid[kMaxKeyLength];  // buffer for subkey name
        DWORD product_guid_len = kMaxKeyLength;
        result = RegEnumKeyExA(hProductsKey.get(), i, product_guid,
                               &product_guid_len, nullptr, nullptr, nullptr,
                               nullptr);
        if (result != ERROR_SUCCESS) {
            std::string error_string = Win32Utils::getErrorString(result);
            printf("RegEnumKeyExA failed %li %s\n", result,
                   error_string.c_str());
            continue;
        }

        std::string install_properties(registry_path);
        install_properties += "\\";
        install_properties += product_guid;
        install_properties += "\\InstallProperties";

        HKEY hkey = 0;
        result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, install_properties.c_str(), 0,
                              samDesired, &hkey);
        if (result != ERROR_SUCCESS) {
            // This is normal at least once
            continue;
        }
        ScopedRegKey hInstallPropertiesKey(hkey);

        DWORD display_name_size = 0;
        const WCHAR displayNameKey[] = L"DisplayName";
        result = RegGetValueW(hInstallPropertiesKey.get(), nullptr,
                              displayNameKey, RRF_RT_REG_SZ | dwGetValueFlags,
                              nullptr, nullptr, &display_name_size);
        if (result != ERROR_SUCCESS && ERROR_MORE_DATA != result) {
            std::string error_string = Win32Utils::getErrorString(result);
            printf("RegGetValueW failed %li %s\n", result,
                   error_string.c_str());
            continue;
        }

        Win32UnicodeString display_name16;
        display_name16.resize(display_name_size / 2);
        result = RegGetValueW(hInstallPropertiesKey.get(), nullptr,
                              displayNameKey, RRF_RT_REG_SZ | dwGetValueFlags,
                              nullptr, display_name16.data(),
                              &display_name_size);
        if (result != ERROR_SUCCESS) {
            std::string error_string = Win32Utils::getErrorString(result);
            printf("RegGetValueW failed %li %s\n", result,
                   error_string.c_str());
            continue;
        }

        std::string display_name8 = display_name16.toString();
        if (0 == strcmp(display_name8.c_str(), productDisplayName)) {
            // We've found the entry for productDisplayName
            DWORD version;
            DWORD version_len = sizeof(version);
            result = RegGetValue(hInstallPropertiesKey.get(), nullptr,
                                 "Version", RRF_RT_DWORD | dwGetValueFlags,
                                 nullptr, &version, &version_len);
            if (result == ERROR_SUCCESS) {
                return (int32_t)version;
            } else {
                // this shouldn't happen
                return kUnknown;
            }
        }
    }

    // productDisplayName not found in any of the subkeys, it's not installed
    return kNotInstalled;
}

}  // namespace android
