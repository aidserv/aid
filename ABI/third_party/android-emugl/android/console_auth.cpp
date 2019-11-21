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

#include "android/base/files/PathUtils.h"
#include "android/base/files/ScopedFd.h"
#include "android/base/misc/FileUtils.h"
#include "android/base/misc/StringUtils.h"
#include "android/base/system/System.h"
#include "android/console_auth.h"
#include "android/console_auth_internal.h"
#include "android/utils/Random.h"
extern "C" {
#include "android/proxy/proxy_int.h"
}
#include "android/utils/debug.h"
#include "android/utils/eintr_wrapper.h"
#include "android/utils/string.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef _WIN32
#define O_BINARY 0
#endif

#define E(...) derror(__VA_ARGS__)

using android::base::System;
using android::base::ScopedFd;

namespace android {
namespace console_auth {

// creates a string suitable for use as an authentication token
// (text that is hard to guess)
bool tokenGenerate(std::string* auth_token) {
    char buf[12];  // 96 bit
    if (!android::generateRandomBytes(buf, sizeof(buf))) {
        return false;
    }

    const size_t kBase64Len = 4 * ((sizeof(buf) + 2) / 3);
    char base64[kBase64Len];
    int err = proxy_base64_encode(buf, sizeof(buf), base64, kBase64Len);
    if (err < 0) {
        E("proxy_base64_encode failed.  (What?)");
        return false;
    }

    auth_token->assign(base64, kBase64Len);
    return true;
}

// reads auth token from the specified path
// if the file does not exist, creates a file at the specified path
// and writes an auth token to it
//
// |path| is a separate parameter to make it unit testable
bool tokenLoadOrCreate(const std::string& path, std::string* auth_token) {
    // it is critical that this file only be readable by the current user
    // to prevent other users on the system from reading it and gaining access
    // to the emulator console
    const mode_t user_read_only = 0600;

    // this open will fail if the file already exists
    ScopedFd fd(HANDLE_EINTR(
            open(path.c_str(), O_CREAT | O_EXCL | O_WRONLY | O_TRUNC | O_BINARY,
                 user_read_only)));
    if (fd.get() < 0) {
        // file already exists, read it
        ScopedFd read(HANDLE_EINTR(open(path.c_str(), O_RDONLY | O_BINARY)));
        if (read.get() < 0) {
            return false;
        }

        std::string file_contents;
        if (!android::readFileIntoString(read.get(), &file_contents)) {
            return false;
        }
        *auth_token = android::base::trim(file_contents);
    } else {
        if (!tokenGenerate(auth_token)) {
            fd.close();
            unlink(path.c_str());
            return false;
        }
        if (!android::writeStringToFile(fd.get(), *auth_token)) {
            E("Oh snap!  couldn't write ~/.emulator_console_auth_token");
            return false;
        }
    }
    return true;
}

// Returns a path to the console auth token file on the current system
std::string tokenGetPathDefault() {
    System* sys = System::get();
    return android::base::PathUtils::join(
            sys->getHomeDirectory(), ".emulator_console_auth_token");
}

fn_get_auth_token_path g_get_auth_token_path = tokenGetPathDefault;

// Gets/creates an auth token in the default path
bool tokenGet(std::string* auth_token) {
    return tokenLoadOrCreate(g_get_auth_token_path(), auth_token);
}

}  // namespace console_auth

}  // namespace android

char* android_console_auth_get_token_dup() {
    std::string auth_token;
    if (!android::console_auth::tokenGet(&auth_token)) {
        // couldn't get auth token, disable the console
        return nullptr;
    }

    return strdup(auth_token.c_str());
}

int android_console_auth_get_status() {
    std::string auth_token;
    if (!android::console_auth::tokenGet(&auth_token)) {
        // couldn't get auth token, disable the console
        return CONSOLE_AUTH_STATUS_ERROR;
    }

    if (auth_token.size() == 0) {
        // an empty auth token string means that auth isn't required
        return CONSOLE_AUTH_STATUS_DISABLED;
    }
    return CONSOLE_AUTH_STATUS_REQUIRED;
}

char* android_console_auth_token_path_dup() {
    std::string auth_token = android::console_auth::g_get_auth_token_path();
    return strdup(auth_token.c_str());
}
