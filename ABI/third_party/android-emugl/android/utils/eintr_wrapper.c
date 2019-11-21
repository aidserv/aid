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

#include "android/utils/eintr_wrapper.h"

#include "android/utils/panic.h"

#ifndef _WIN32
void android_eintr_wrapper_fatal(const char* file,
                                 long lineno,
                                 const char* function,
                                 const char* call) {
    android_panic(
            "%s:%ld:%s%s System call looped around EINTR %d times: %s\n",
            file,
            lineno,
            function ? function : "",
            function ? ":" : "",
            MAX_EINTR_LOOP_COUNT,
            call);
}
#endif  // !_WIN32
