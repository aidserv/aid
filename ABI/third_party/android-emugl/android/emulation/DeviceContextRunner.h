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

#include "android/base/Log.h"
#include "android/base/async/Looper.h"
#include "android/base/async/ThreadLooper.h"
#include "android/base/synchronization/Lock.h"
#include "android/emulation/VmLock.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <vector>

namespace android {
// All operations that change the global VM state (e.g.
// virtual device operations) should happen in a thread
// that holds the global VM lock.
//
// DeviceContextRunner is a helper template class used
// to ensure that a given operation is always performed
// in such a thread. more specifically:
// - If the current thread already owns the lock,
//   the operation is performed as-is.
// - Otherwise, it is queued and will be run in the
//   main-loop thread as soon as possible.
//
// Usage is the following:
//
// - Define a custom type |OP| corresponding to
//   the state of each operation. It must be copyable.
//
// - Define a derived class of
//   |DeviceContextRunner<OP>| that must implement the
//   abstract performDeviceOperation(const OP& op) method.
//
// - Create a DeviceContextRunner<OP> instance, and call
//   its init() method, passing a valid android::VmLock
//   instance to it. NOTE: This should be called from
//   the main loop thread, during emulation setup time!!
//
// - Whenever you want to perform an operation, call
//   queueDeviceOperation(<op>) on it. If the current
//   thread holds the lock, it will be
//   performed immediately. Otherwise, it will be queued
//   and run later. Hence the void return type of
//   queueDeviceOperation; it is run asynchronously, so
//   you cannot expect a return value.

enum class ContextRunMode {
    DeferIfNotLocked,
    DeferAlways
};

template <typename T>
class DeviceContextRunner {
public:
    using AutoLock = android::base::AutoLock;
    using Lock = android::base::Lock;
    using Looper = android::base::Looper;
    using ThreadLooper = android::base::ThreadLooper;
    using VmLock = android::VmLock;
    using PendingList = std::vector<T>;

    void init(VmLock* vmLock) { init(vmLock, ThreadLooper::get()); }

    // Looper parameter is for unit testing purposes.
    void init(VmLock* vmLock, Looper* looper) {
        mVmLock = vmLock;
        // TODO(digit): Find a better event abstraction.
        //
        // Operating on Looper::Timer objects is not supposed to be
        // thread-safe, but it appears that their QEMU1 and QEMU2 specific
        // implementation *is* (see qemu-timer.c:timer_mod()).
        //
        // This means that in practice the code below is safe when used
        // in the context of an Android emulation engine. However, we probably
        // need a better abstraction that also works with the generic
        // Looper implementation (for unit-testing) or any other kind of
        // runtime environment, should we one day link AndroidEmu to a
        // different emulation engine.
        mTimer.reset(looper->createTimer(
                [](void* that, Looper::Timer*) {
                    static_cast<DeviceContextRunner*>(that)->onTimerEvent();
                },
                this));
        if (!mTimer.get()) {
            LOG(FATAL) << "Failed to create a loop timer in DeviceContextRunner";
        }
    }

    void setContextRunMode(ContextRunMode mode) {
        AutoLock lock(mLock);
        mContextRunMode = mode;
    }

protected:
    // Disable delete-through-interface.
    ~DeviceContextRunner() = default;

    // To be implemented by the class that derives DeviceContextRunner:
    // the method that actually touches the virtual device.
    virtual void performDeviceOperation(const T& op) = 0;

    // queueDeviceOperation: If the VM lock is currently held,
    // we are OK to actually perform device operations.
    // Otherwise, we need to add the request to a pending
    // set of requests, to be finished later when we do have the VM lock.
    void queueDeviceOperation(const T& op) {
        if (mContextRunMode == ContextRunMode::DeferIfNotLocked &&
            mVmLock->isLockedBySelf()) {
            // Perform the operation correctly since the current thread
            // already holds the lock that protects the global VM state.
            performDeviceOperation(op);
        } else {
            // Queue the operation in the mPendingMap structure, then
            // restart the timer.
            AutoLock lock(mLock);
            mPending.push_back(op);
            lock.unlock();

            // NOTE: See TODO above why this is thread-safe when used with
            // QEMU1 and QEMU2.
            mTimer->startAbsolute(0);
        }
    }

    // Remove all pending operations that match the passed predicate |op|.
    template <class Predicate>
    void removeAllPendingOperations(const Predicate& op) {
        AutoLock lock(mLock);
        mPending.erase(std::remove_if(mPending.begin(), mPending.end(), op),
                       mPending.end());
    }

    // Run the passed functor |op| for all pending operations.
    template <class Func>
    void forEachPendingOperation(const Func& op) const {
        AutoLock lock(mLock);
        for (const auto& p : mPending) { op(p); }
    }

private:
    void onTimerEvent() {
        AutoLock lock(mLock);
        for (const auto& elt : mPending) {
            performDeviceOperation(elt);
        }
        mPending.clear();
    }

    VmLock* mVmLock = nullptr;
    ContextRunMode mContextRunMode = ContextRunMode::DeferIfNotLocked;

    mutable Lock mLock;
    PendingList mPending;
    std::unique_ptr<Looper::Timer> mTimer;
};

}  // namespace android
