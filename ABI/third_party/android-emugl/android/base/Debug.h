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

#include "android/base/system/System.h"

// A macro to define piece of code only for debugging. This is useful when
// checks are much bigger than a single assert(), e.g.:
//  class Lock {
//      ANDROID_IF_DEBUG(bool isLocked() { ... })
//      ...
//  };
//
//  void foo::bar() { assert(mLock.isLocked()); }
//
// Lock::isLocked() won't exist in release binaries at all.
//
#ifdef NDEBUG
#define ANDROID_IF_DEBUG(x)
#else   // !NDEBUG
#define ANDROID_IF_DEBUG(x) x
#endif  // !NDEBUG

namespace android {
namespace base {

// Checks if the current process has a debugger attached.
bool IsDebuggerAttached();

// Suspends the current process for up to |timeoutMs| until a debugger attaches.
// |timeoutMs| == -1 means 'no timeout'
bool WaitForDebugger(System::Duration timeoutMs = -1);

// Issues a break into debugger (if one attached). Without a debugger it could
// do anything, but most probably will crash.
void DebugBreak();

}  // namespace base
}  // namespace android
