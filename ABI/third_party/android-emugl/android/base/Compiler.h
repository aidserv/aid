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

// Use this inside a class declaration to ensure that the corresponding objects
// cannot be copy-constructed or assigned. For example:
//
//   class Foo {
//       ....
//       DISALLOW_COPY_AND_ASSIGN(Foo)
//       ....
//   };
//
// Note: this macro is sometimes defined in 3rd-party libs, so let's check first
#ifndef DISALLOW_COPY_AND_ASSIGN

#define DISALLOW_COPY_AND_ASSIGN(T) \
    T(const T& other) = delete; \
    T& operator=(const T& other) = delete

#endif

#ifndef DISALLOW_COPY_ASSIGN_AND_MOVE

#define DISALLOW_COPY_ASSIGN_AND_MOVE(T) \
    DISALLOW_COPY_AND_ASSIGN(T); \
    T(T&&) = delete; \
    T& operator=(T&&) = delete

#endif
