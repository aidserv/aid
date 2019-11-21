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

#include "android/utils/compiler.h"
#include <stdlib.h>

ANDROID_BEGIN_HEADER

char* android_console_auth_token_path_dup();

char* android_console_auth_get_token_dup();

#define CONSOLE_AUTH_STATUS_REQUIRED \
    0  // ~/.emulator_console_auth_token contains an auth token
#define CONSOLE_AUTH_STATUS_DISABLED \
    1  // ~/.emulator_console_auth_token is empty
#define CONSOLE_AUTH_STATUS_ERROR \
    2  // ~/.emulator_console_auth_token is inaccessible

int android_console_auth_get_status();

ANDROID_END_HEADER
