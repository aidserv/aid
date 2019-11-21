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

#include "android/utils/debug.h"
#include "android/utils/stringify.h"

//
// A set of useful debug logging macros for the whole metrics/ subdirectory.
//

#define D(format, ...) \
        VERBOSE_PRINT(metrics, \
            "(metrics::%s) " format, __func__, ##__VA_ARGS__)

#define W(format, ...) \
    do { \
        dwarning("(metrics) " format, ##__VA_ARGS__); \
    } while (0)

#define E(format, ...) \
    do { \
        derror("(metrics) " format, ##__VA_ARGS__); \
    } while (0)
