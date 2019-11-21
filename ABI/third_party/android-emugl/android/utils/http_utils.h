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

#pragma once

#include "android/utils/compiler.h"

#include <stdbool.h>
#include <stddef.h>

ANDROID_BEGIN_HEADER

// Simple C wrapper for android::base::httpIsRequestLine().
bool android_http_is_request_line(const char* line,
                                  size_t line_len);

ANDROID_END_HEADER
