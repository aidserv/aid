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

#include "android/utils/file_io.h"
#include "android/base/files/Fd.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/base/system/Win32UnicodeString.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>

using android::base::Win32UnicodeString;
using android::base::ScopedCPtr;
#endif

// Provide different macros for different number of string arguments where each
// string argument requires conversion
#ifdef _WIN32
#define WIDEN_CALL_1(func, str1, ...) \
      _w ## func(Win32UnicodeString(str1).c_str() , ##__VA_ARGS__)
#define WIDEN_CALL_2(func, str1, str2, ...) \
      _w ## func(Win32UnicodeString(str1).c_str(), \
                 Win32UnicodeString(str2).c_str() , ##__VA_ARGS__)
#else
#define WIDEN_CALL_1(func, str1, ...) func(str1 , ##__VA_ARGS__)
#define WIDEN_CALL_2(func, str1, str2, ...) func(str1, str2 , ##__VA_ARGS__)
#endif

extern "C" {

FILE* android_fopen(const char* path, const char* mode) {
    return WIDEN_CALL_2(fopen, path, mode);
}

FILE* android_popen(const char* path, const char* mode) {
    return WIDEN_CALL_2(popen, path, mode);
}

int android_open_without_mode(const char* path, int flags) {
    int res = WIDEN_CALL_1(open, path, flags | O_CLOEXEC);
    android::base::fdSetCloexec(res);
    return res;
}

int android_open_with_mode(const char* path, int flags, mode_t mode) {
    int res = WIDEN_CALL_1(open, path, flags | O_CLOEXEC, mode);
    android::base::fdSetCloexec(res);
    return res;
}

int android_stat(const char* path, struct stat* buf) {
#ifdef _WIN32
    // Make sure we use the stat call that matches the type of stat struct we
    // are getting. The incoming struct is a "struct _stati64" and this is the
    // matching call for that struct. Unfortunately the macro doesn't take care
    // of that.
    return _wstati64(Win32UnicodeString(path).c_str(), buf);
#else
    return stat(path, buf);
#endif
}

int android_lstat(const char* path, struct stat* buf) {
#ifdef _WIN32
    // Windows doesn't seem to have an lstat function so just use regular stat
    return android_stat(path, buf);
#else
    return lstat(path, buf);
#endif
}

int android_access(const char* path, int mode) {
    return WIDEN_CALL_1(access, path, mode);
}

#ifdef _WIN32
// The Windows version does not have a mode parameter so just drop that name to
// avoid compiler warnings about unused parameters
int android_mkdir(const char* path, mode_t) {
    return _wmkdir(Win32UnicodeString(path).c_str());
}
#else
int android_mkdir(const char* path, mode_t mode) {
    return mkdir(path, mode);
}
#endif

int android_creat(const char* path, mode_t mode) {
#ifndef _WIN32
    return android_open(path, O_CREAT | O_WRONLY | O_TRUNC, mode);
#else
    int res = WIDEN_CALL_1(creat, path, mode);
    android::base::fdSetCloexec(res);
    return res;
#endif
}

int android_unlink(const char* path) {
    return WIDEN_CALL_1(unlink, path);
}

int android_chmod(const char* path, mode_t mode) {
    return WIDEN_CALL_1(chmod, path, mode);
}

// The code below uses the fact that GCC supports something called weak linking.
// Several functions in glibc are weakly linked which means that if the same
// function name is found in the application binary that function will be used
// instead. On Windows we use this to change these calls to call the wchar_t
// equivalent function with the parameter converted from UTF-8 to UTF-16.
//
// Unfortunately stat is not weakly linked which is why it is not listed here
// and any code that calls stat should use android_stat instead.
#ifdef _WIN32

// getcwd cannot use the same macro as the other calls because it places data
// in one of its parameters and it returns a char pointer, not a result code
char* __cdecl getcwd(char* buffer, int maxlen) {
    ScopedCPtr<wchar_t> wideCwd(_wgetcwd(nullptr, 0));
    if (wideCwd.get() == nullptr) {
        return nullptr;
    }
    if (buffer == nullptr) {
        // This is a valid use case and we need to allocate memory and return it
        auto narrowCwd = Win32UnicodeString::convertToUtf8(wideCwd.get());
        return strdup(narrowCwd.c_str());
    }

    int written = Win32UnicodeString::convertToUtf8(buffer,
                                                    maxlen,
                                                    wideCwd.get());
    if (written < 0 || written >= maxlen) {
        return nullptr;
    }
    return buffer;
}

int __cdecl remove(const char* path) {
    return WIDEN_CALL_1(remove, path);
}

int __cdecl rmdir(const char* dirname) {
    return WIDEN_CALL_1(rmdir, dirname);
}

int __cdecl chmod(const char* filename, int pmode) {
    return WIDEN_CALL_1(chmod, filename, pmode);
}

int __cdecl unlink(const char* filename) {
    return WIDEN_CALL_1(unlink, filename);
}

int __cdecl mkdir(const char* dirname) {
    return WIDEN_CALL_1(mkdir, dirname);
}

int __cdecl creat(const char* path, int mode) {
    return WIDEN_CALL_1(creat, path, mode);
}

int __cdecl access(const char* path, int mode) {
    return WIDEN_CALL_1(access, path, mode);
}

int __cdecl open(const char* pathname, int flags, ...) {
    va_list ap;

    // Since open comes in two versions and C does not support function
    // overloading we use varargs for the mode parameters instead.
    va_start(ap, flags);
    int result = WIDEN_CALL_1(open, pathname, flags, va_arg(ap, int));
    va_end(ap);
    return result;
}

FILE* __cdecl fopen(const char* path, const char* mode) {
    return WIDEN_CALL_2(fopen, path, mode);
}

#endif  // _WIN32

}  // extern "C"

