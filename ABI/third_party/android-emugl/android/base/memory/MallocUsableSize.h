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

// Define HAVE_MALLOC_USABLE_SIZE to 1 to indicate that the current
// system has malloc_usable_size(). Which takes the address of a malloc-ed
// pointer, and return the size of the underlying storage block.
// This is useful to optimize heap memory usage.

// Including at least one C library header is required to define symbols
// like __GLIBC__. Choose carefully because some headers like <stddef.h>
// are actually provided by the compiler, not the C library and do not
// define the macros we need.
#include <stdint.h>

#if defined(__GLIBC__)
#  include <malloc.h>
#  define USE_MALLOC_USABLE_SIZE  1
#elif defined(__APPLE__) || defined(__FreeBSD__)
#  include <malloc/malloc.h>
#  define malloc_usable_size  malloc_size
#  define USE_MALLOC_USABLE_SIZE  1
#else
#  define USE_MALLOC_USABLE_SIZE  0
#endif
