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

#include <errno.h>

#include "android/base/Log.h"

namespace android {
namespace base {

// Set EINTR_WRAPPER_DEBUG to 1 to force the debug version of HANDLE_EINTR
// which will call eintrWrapperFatal() is the system call loops
// too many times, or 0 to get it to loop indefinitly.
// Mostly used for unit testing.
// If the macro is undefined, auto-detect the value based on NDEBUG.
#if !defined(EINTR_WRAPPER_DEBUG)
#  ifdef NDEBUG
#    define EINTR_WRAPPER_DEBUG 0
#  else
#    define EINTR_WRAPPER_DEBUG 1
#  endif
#endif

// HANDLE_EINTR() is a macro used to handle EINTR return values when
// calling system calls like open() or read() on Posix systems.
//
// By default, this will loop indefinitly, retrying the call until
// the result is no longer -1/EINTR, except in debug mode, where a
// loop counter is actually used and to provoke a fatal error if there
// are too many loops.
//
// Usage example:
//     int ret = HANDLE_EINTR(open("/some/file/path", O_RDONLY));
//
// IMPORTANT: Do not use with the close() system call (use IGNORE_EINTR()
// instead).
//
// - On Linux, the file descriptor is always already closed when this
//   function returns -1/EINTR, and calling it again with the same
//   parameters risks closing another file descriptor open by another
//   thread in parallel!
//
// - On OS X, whether the file descriptor is closed or not is pretty
//   much random! It's better to leave the descriptor open than risk
//   closing another one by mistake :(
//
#define MAX_EINTR_LOOP_COUNT  100

#ifdef _WIN32
#  define HANDLE_EINTR(x)  (x)
#elif EINTR_WRAPPER_DEBUG == 0
#  define HANDLE_EINTR(x) \
    __extension__ ({ \
        __typeof__(x) eintr_wrapper_result; \
        do { \
            eintr_wrapper_result = (x); \
        } while (eintr_wrapper_result < 0 && errno == EINTR); \
        eintr_wrapper_result; \
    })
#else  // !_WIN32 && EINTR_WRAPPER_DEBUG

#  define HANDLE_EINTR(x) \
    __extension__ ({ \
        __typeof__(x) eintr_wrapper_result; \
        int eintr_wrapper_loop_count = 0; \
        for (;;) { \
            eintr_wrapper_result = (x); \
            if (eintr_wrapper_result != -1 || errno != EINTR) \
                break; \
            ++eintr_wrapper_loop_count; \
            CHECK(eintr_wrapper_loop_count < MAX_EINTR_LOOP_COUNT) << \
                "Looping around EINTR too many times"; \
        }; \
        eintr_wrapper_result; \
    })
#endif  // !_WIN32 && EINTR_WRAPPER_DEBUG

// IGNORE_EINTR() is a macro used to perform a system call and ignore
// an EINTR result, i.e. it will return 0 instead of -1 if this occurs.
// This is mostly used with the close() system call, as described
// in the HANDLE_EINTR() documentation.
#ifdef _WIN32
#  define IGNORE_EINTR(x)  (x)
#else
#  define IGNORE_EINTR(x) \
    __extension__ ({ \
        __typeof__(x) eintr_wrapper_result = (x); \
        if (eintr_wrapper_result == -1 && errno == EINTR) \
            eintr_wrapper_result = 0; \
        eintr_wrapper_result; \
    })
#endif

}  // namespace base
}  // namespace android
