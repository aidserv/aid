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

// Let's start off our process in style.
// - This initialization needs to happen before any threads are launched;
// - After this initialization, any crashes will be reported (if enabled).
void process_early_setup(int argc, char** argv);

void process_late_teardown();

ANDROID_END_HEADER
