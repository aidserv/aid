// Copyright 2016 The Android Open Source Project
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

#include "android/opengl/emugl_config.h"
#include "android/skin/winsys.h"
#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

// Convenience function used to initialize an EmuglConfig instance |config|
// with appropriate settings corresponding to an AVD startup configuration.
// |avdName| is the AVD name, or nullptr to indicate a platform build.
// |avdArch| is the AVD architecture (e.g. 'arm64')
// |apiLevel| is the AVD API level.
// |hasGoogleApis| should be true iff the system image provides Google APIs.
// |gpuOption| is the value of the '-gpu' option, if any.
// |wantedBitness| is the wanted bitness of the emulation engine. A value
// of 0 means use the current program's bitness.
// |noWindow| is true iff the -no-window option was used.
// |uiPreferredBackend| communicates the preferred GLES backend from the UI.
// The UI setting can be overridden if the user is logging in through remote desktop.
// On success, initializes |config| and returns true. Return false on failure.
bool androidEmuglConfigInit(EmuglConfig* config,
                            const char* avdName,
                            const char* avdArch,
                            int apiLevel,
                            bool hasGoogleApis,
                            const char* gpuOption,
                            int wantedBitness,
                            bool noWindow,
                            enum WinsysPreferredGlesBackend uiPreferredBackend);

ANDROID_END_HEADER
