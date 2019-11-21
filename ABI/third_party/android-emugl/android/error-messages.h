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

#include <stdbool.h>

ANDROID_BEGIN_HEADER

// Set an error code and message for an error that occurred during
// initialization. The function will make a copy of the string in
// |error_message|. If |error_message| is NULL then the string kUnknownInitError
// will be used instead.
void android_init_error_set(int error_code, const char* error_message);

// Indicate if an init error occurred or not
bool android_init_error_occurred(void);

// Get the error code set by android_set_init_error
int android_init_error_get_error_code(void);

// Get the error message set by android_set_init_error. If
// android_init_error_set has never been called this will return NULL.
const char* android_init_error_get_message(void);


/*** Error message ***/

// Error message to explain why haxm's vcpu sync failed
extern const char* const kHaxVcpuSyncFailed;

// Error message indicating that some unknown error occurred during startup
extern const char* const kUnknownInitError;

// Format string for an error message indicating not enough RAM for QEMU guest
extern const char* const kNotEnoughMemForGuestError;

ANDROID_END_HEADER
