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

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

ANDROID_BEGIN_HEADER

FILE* android_fopen(const char* path, const char* mode);
FILE* android_popen(const char* path, const char* mode);

// This thing uses macro varargs to select which open function to use.
// Because __VA_ARGS__ consumes a different amount of arguments the call_name
// will have a different name depending on how many arguments are passed
#define DETERMINE_OPEN_CALL(first, second, third, call_name, ...) call_name
#define android_open(...) DETERMINE_OPEN_CALL(__VA_ARGS__, \
                                              android_open_with_mode, \
                                              android_open_without_mode) \
                                                  (__VA_ARGS__)

// WARNING: Do not use these, just use the android_open macro above the way
// you would use open. The version to use gets picked automatically
int android_open_without_mode(const char* path, int flags);
int android_open_with_mode(const char* path, int flags, mode_t mode);

int android_stat(const char* path, struct stat* buf);
int android_lstat(const char* path, struct stat* buf);

int android_access(const char* path, int mode);
int android_mkdir(const char* path, mode_t mode);

int android_creat(const char* path, mode_t mode);

int android_unlink(const char* path);
int android_chmod(const char* path, mode_t mode);

ANDROID_END_HEADER

