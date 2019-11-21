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
namespace console_auth {

bool tokenLoadOrCreate(const std::string& path, std::string* auth_token);

std::string tokenGetPathDefault();

// For unit test mocking only.  This lets unit tests run with a temp file
// instead of the user's real auth token
typedef std::string (*fn_get_auth_token_path)();
extern fn_get_auth_token_path g_get_auth_token_path;

}  // namespace console_auth
}  // namespace android
