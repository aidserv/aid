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

#include "android/utils/debug.h"

#include "android/base/Log.h"

void base_enable_verbose_logs() {
    android_verbose = ~0;
    android::base::setMinLogLevel(android::base::LOG_VERBOSE);
}

void base_disable_verbose_logs() {
    android_verbose = 0;
    android::base::setMinLogLevel(android::base::LOG_INFO);
}
