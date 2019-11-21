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

#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

// Try to parse |option| as a command-line help option, which can be "-help"
// or "-help-<option>" or "-help-<topic>". The following cases are possible:
// - |option| is not '-help' nor starts with '-help-', the function returns -1.
// - |option| is '-help-<unknown>' where <unknown> is an unknown option or
//   help topic. The function prints an error message to stderr and returns
//   an exit status of 1.
// - otherwise, it prints the help to stdout and returns an exit status of 0.
int emulator_parseHelpOption(const char* option);

ANDROID_END_HEADER
