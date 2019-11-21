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

#include "android/utils/http_utils.h"

#include "android/base/misc/HttpUtils.h"

bool android_http_is_request_line(const char* line,
                                  size_t line_len) {
    return android::base::httpIsRequestLine(line, line_len);
}
