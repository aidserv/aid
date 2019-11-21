// Copyright 2014 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/utils/win32_cmdline_quote.h"

#include "android/base/misc/StringUtils.h"
#include "android/base/system/Win32Utils.h"

#include "android/utils/system.h"

char* win32_cmdline_quote(const char* param) {
    std::string result = android::base::Win32Utils::quoteCommandLine(param);
    return android::base::strDup(result);
}
