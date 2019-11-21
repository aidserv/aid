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

ANDROID_BEGIN_HEADER

// Quote a given command-line command or parameter so that it can be
// sent to _execv() on Windows, and be received properly by the new
// process.
//
// This is necessary to work-around an annoying issue on Windows, where
// spawn() / exec() functions are mere wrappers around CreateProcess, which
// always pass the command-line as a _single_ string made of the simple
// concatenations of their arguments, while the new process will typically
// use CommandLineToArgv to convert it into a list of command-line arguments,
// expected the values to be quoted properly.
//
// For more details about this mess, read the MSDN blog post named
// "Everyone quotes arguments the wrong way".
//
// |param| is an input string, that may contain spaces, quotes or backslashes.
// The function returns a new heap-allocated function that contains a version
// of |param| that can be decoded properly by CommandLineToArgv(). The caller
// must free the string with android_free() or AFREE().
char* win32_cmdline_quote(const char* param);

ANDROID_END_HEADER
