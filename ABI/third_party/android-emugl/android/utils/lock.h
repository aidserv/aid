// Copyright 2016 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

// A thin wrapper around platform independent locks.
// This header is intended to be used in legacy C code.  Almost always, you want
// something else:
// - AndroidEmu should mostly be c++, and use
//   android/base/synchronization/Lock.h
// - Qemu code should use qemu's own platform independent objects.

// To use:
// CLock* l = android_lock_new();
// android_lock_acquire(l);
// /* Now do important stuff */
// android_lock_release(l);
// android_lock_free(l);
typedef struct CLock CLock;

extern CLock* android_lock_new();
extern void android_lock_acquire(CLock* l);
extern void android_lock_release(CLock* l);
extern void android_lock_free(CLock* l);

ANDROID_END_HEADER
