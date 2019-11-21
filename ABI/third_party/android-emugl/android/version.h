// Copyright 2015 The Android Open Source Project
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

#include "android/utils/stringify.h"

// Define the common emulator version strings
#ifdef ANDROID_SDK_TOOLS_REVISION
#  define  EMULATOR_VERSION_STRING_SHORT  STRINGIFY(ANDROID_SDK_TOOLS_REVISION)
#  define  EMULATOR_VERSION_STRING        EMULATOR_VERSION_STRING_SHORT ".0"
#else
#  define  EMULATOR_VERSION_STRING_SHORT  "standalone"
#  define  EMULATOR_VERSION_STRING        EMULATOR_VERSION_STRING_SHORT
#endif

// Define the emulator build number string
#ifdef ANDROID_SDK_TOOLS_BUILD_NUMBER
#  define EMULATOR_BUILD_STRING STRINGIFY(ANDROID_SDK_TOOLS_BUILD_NUMBER)
#else
#  define EMULATOR_BUILD_STRING "0"
#endif

// Latest CL in the emulator build
#ifdef ANDROID_SDK_TOOLS_CL_SHA1
#  define EMULATOR_CL_SHA1 STRINGIFY(ANDROID_SDK_TOOLS_CL_SHA1)
#else
#  define EMULATOR_CL_SHA1 "N/A"
#endif

// Full version, with build suffix
#define EMULATOR_FULL_VERSION_STRING \
    EMULATOR_VERSION_STRING_SHORT "-" EMULATOR_BUILD_STRING
