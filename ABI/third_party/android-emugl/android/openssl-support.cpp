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

#include "android/openssl-support.h"

#include "android/base/synchronization/Lock.h"
#include "android/base/threads/Thread.h"

#include <assert.h>
#include <openssl/crypto.h>

using android::base::Lock;
using android::base::Thread;

unsigned sNumLocks = 0;
Lock* sOpenSslLocks = nullptr;

static void locking_callback(int mode, int n,  const char* file, int line) {
    assert(n >= 0 && (unsigned)n < sNumLocks);
    if (mode & CRYPTO_LOCK) {
        sOpenSslLocks[n].lock();
    } else {
        sOpenSslLocks[n].unlock();
    }
}

bool android_openssl_init() {
    sNumLocks = CRYPTO_num_locks();
    delete[] sOpenSslLocks;
    sOpenSslLocks = new Lock[sNumLocks];
    if (!sOpenSslLocks) {
        sNumLocks = 0;
        return false;
    }

    CRYPTO_set_locking_callback(locking_callback);
    return true;
}

void android_openssl_finish() {
    CRYPTO_set_locking_callback(nullptr);

    delete[] sOpenSslLocks;
    sOpenSslLocks = nullptr;
    sNumLocks = 0;
}
