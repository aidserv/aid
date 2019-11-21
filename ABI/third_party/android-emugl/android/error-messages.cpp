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

#include "android/error-messages.h"

#include <string>

extern "C" {

// Keep track of if an error was actually set, this way we don't have to encode
// this state using a magic number in the actual error code.
static bool init_error_occurred = false;
static int init_error_code = 0;
static std::string init_error_message;

// Select an error message describing the error. If the provided message is a
// nullptr or an empty string we use a default string instead to ensure that the
// user gets some kind of valuable information instead of just an empty error.
static const char* selectErrorMessage(const char* message) {
    if (message == nullptr || message[0] == '\0') {
        return kUnknownInitError;
    }
    return message;
}

void android_init_error_set(int error_code, const char* error_message) {
    init_error_occurred = true;
    init_error_code = error_code;
    init_error_message = selectErrorMessage(error_message);
}

// NOTE: this function is not declared in the header file because you are not
// supposed to use it. Once an init error occurred then that's the end of the
// line. This only exists so that unit tests can start out with a clean slate.
void android_init_error_clear() {
    init_error_occurred = false;
    init_error_code = 0;
    init_error_message.clear();
}

bool android_init_error_occurred(void) {
    return init_error_occurred;
}

int android_init_error_get_error_code(void) {
    return init_error_code;
}

const char* android_init_error_get_message(void) {
    if (android_init_error_occurred()) {
        return init_error_message.c_str();
    }
    return nullptr;
}

const char* const kHaxVcpuSyncFailed =
        "Unfortunately, there's an incompatibility between HAXM hypervisor and "
        "VirtualBox 4.3.30+ which doesn't allow multiple hypervisors "
        "to co-exist.  It is being actively worked on; you can find out more "
        "about the issue at http://b.android.com/197915 (Android) and "
        "https://www.virtualbox.org/ticket/14294 (VirtualBox)";

const char* const kUnknownInitError =
        "An unknown error occured when starting Android Emulator. Please "
        "consider filing a bug report describing what happened";

const char* const kNotEnoughMemForGuestError =
        "Android Emulator could not allocate %.1f GB "
        "of memory for the current AVD configuration. "
        "Consider adjusting the RAM size of your AVD "
        "in the AVD Manager.\n\n"
        "Error detail: QEMU '%s'";

}  // extern "C"
