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

#include "android/cmdline-option.h"
#include "android/utils/compiler.h"
#include "android/opengl/emugl_config.h"

#include <stdint.h>

ANDROID_BEGIN_HEADER

// Return a heap-allocated string containing the kernel parameter.
// |opts| corresponds to the command-line options after they have been
// processed by emulator_parseCommonCommandLineOptions().
// |targetArch| is the target architecture. (e.g. 'arm64')
// |apiLevel| is the AVD's API level.
// |kernelSerialPrefix| is the guest tty device prefix (e.g. 'ttyS')
// |avdKernelParameters| are the optional extra kernel parameters stored
// in the AVD's kernel.parameters hardware property, if any. They will
// be appended to the result.
// |glesMode| is the EGL/GLES emulation mode.
// |glesGuestCmaMB| is the size in megabytes of the contiguous memory
// allocation to be used when |glesMode| is kAndroidGlesEmulationGuest.
// A value of 0 also indicates to ignore this setting.
// |isQemu2| is true to indicate that this is called from QEMU2, otherwise
// QEMU1 is assumed.
char* emulator_getKernelParameters(const AndroidOptions* opts,
                                   const char* targetArch,
                                   int apiLevel,
                                   const char* kernelSerialPrefix,
                                   const char* avdKernelParameters,
                                   AndroidGlesEmulationMode glesMode,
                                   int bootPropOpenglesVersion,
                                   uint64_t glFramebufferSizeBytes,
                                   bool isQemu2);

ANDROID_END_HEADER
