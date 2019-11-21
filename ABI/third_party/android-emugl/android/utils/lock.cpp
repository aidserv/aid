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

#include "android/utils/lock.h"

#include "android/base/synchronization/Lock.h"

using android::base::Lock;

static Lock* asBaseLock(CLock* l) {
    return reinterpret_cast<Lock*>(l);
}

CLock* android_lock_new() {
    return reinterpret_cast<CLock*>(new Lock());
}

void android_lock_acquire(CLock* l) {
    if (l) {
        asBaseLock(l)->lock();
    }
}

void android_lock_release(CLock* l) {
    if (l) {
        asBaseLock(l)->unlock();
    }
}

void android_lock_free(CLock* l) {
    delete asBaseLock(l);
}
