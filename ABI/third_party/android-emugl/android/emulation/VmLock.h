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

#include "android/base/Compiler.h"

namespace android {

// In QEMU2, each virtual CPU runs on its own host threads, but all these
// threads are synchronized through a global mutex, which allows the virtual
// device code to not care about them.
//
// However, if you have to call, from any other thread, a low-level QEMU
// function that operate on virtual devices (e.g. some Android pipe-related
// functions), you must acquire the global mutex before doing so, and release
// it after that.

// This header provides a convenience interface class you can use to do
// just that, i.e.:
//
// 1) To operate on the lock, call VmLock::get() to retrieve the
//    current VmLock instance, then invoke its lock() and unlock()
//    methods.
//
// 2) Glue code should call VmLock::set() to inject their own implementation
//    into the process. The default implementation doesn't do anything.
//clear


class VmLock {
public:
    // Constructor.
    VmLock() = default;

    // Destructor.
    virtual ~VmLock() = default;

    // Lock the VM global mutex.
    virtual void lock() {}

    // Unlock the VM global mutex.
    virtual void unlock() {}

    // Returns true iff the lock is held by the current thread, false
    // otherwise. Note that for a correct implementation, that doesn't
    // only depend on the number of times that VmLock::lock() and
    // VmLock::unlock() were called, but also on other QEMU threads that
    // act on the global lock.
    virtual bool isLockedBySelf() const { return true; }

    // Return current VmLock instance. Cannot return nullptr.
    // NOT thread-safe, but we don't expect multiple threads to call this
    // concurrently at init time, and the worst that can happen is to leak
    // a single instance.
    static VmLock* get();

    // Set new VmLock instance. Return old value, which cannot be nullptr and
    // can be deleted by the caller. If |vmLock| is nullptr, a new default
    // instance is created. NOTE: not thread-safe with regards to get().
    static VmLock* set(VmLock* vmLock);

    DISALLOW_COPY_ASSIGN_AND_MOVE(VmLock);
};

// Convenience class to perform scoped VM locking.
class ScopedVmLock {
public:
    ScopedVmLock(VmLock* vmLock = VmLock::get()) : mVmLock(vmLock) {
        mVmLock->lock();
    }

    ~ScopedVmLock() {
        mVmLock->unlock();
    }

    DISALLOW_COPY_ASSIGN_AND_MOVE(ScopedVmLock);

private:
    VmLock* mVmLock;
};

// Another convenience class for a code that may run either under a lock or not
// but needs to ensure that some part of it runs without a VmLock.
class ScopedVmUnlock {
public:
    ScopedVmUnlock(VmLock* vmLock = VmLock::get()) {
        if (vmLock->isLockedBySelf()) {
            mVmLock = vmLock;
            mVmLock->unlock();
        } else {
            mVmLock = nullptr;
        }
    }

    ~ScopedVmUnlock() {
        if (mVmLock) {
            mVmLock->lock();
        }
    }

    DISALLOW_COPY_ASSIGN_AND_MOVE(ScopedVmUnlock);

private:
    VmLock* mVmLock;
};

}  // namespace android
