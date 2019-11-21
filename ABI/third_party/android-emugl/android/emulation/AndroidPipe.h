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

#include "android/base/files/Stream.h"
#include "android/emulation/android_pipe_common.h"
#include "android/emulation/VmLock.h"

namespace android {

// AndroidPipe is a mostly-abstract base class for Android pipe connections.
// It's important to understand that most pipe operations (onGuestXXX methods)
// are performed with the global VM lock held.
//
// - QEMU1 runs all CPU-related operations and all timers/io events on its
//   main loop. This will be referred as the 'device thread' here.
//
// - QEMU2 has dedicated CPU threads to run guest read/write operations and
//   a different main-loop thread to run timers, check for pending host I/O.
//   It uses a global lock that is held every time virtual device operations
//   are performed. The 'device thread' is whichever CPU thread is holding
//   the lock.
//
// IMPORTANT: Most AndroidPipe methods must be called from a thread that holds
// the global VM state lock (i.e. where VmLock::isLockedBySelf() is true).
// Failure to do so will result in incorrect results, and panics in debug
// builds.
//
// A few methods can be called from any thread though, see signalWake() and
// closeFromHost().
//
// Usage is the following:
//
// 1) At emulation setup time (i.e. before the VM runs), call
//    AndroidPipe::initThreading() from the main loop thread and pass an
//    appropriate VmLock instance.
//
// 2) For each supported pipe service, create a new AndroidPipe::Service
//    instance, and register it globally by calling AndroidPipe::Service::add().
//    This can happen before 1) and in different threads.
//
// 3) Once the VM starts, whenever a guest wants to connect to a specific
//    named service, its instance's 'create' method will be called from the
//    device thread. Alternatively, when resuming from a snapshot, the 'load'
//    method will be called iff 'canLoad()' is overloaded to return true.
//
// 4) During pipe operations, onGuestXXX() methods will be called from the
//    device thread to operate on the pipe.
//
// 5) The signalWake() and closeFromHost() pipe methods can be called from
//    any thread to signal i/o events, or ask for the pipe closure.
//
class AndroidPipe {
public:
    // Call this function in the main loop thread to ensure proper threading
    // support. |vmLock| must be a valid android::VmLock instance that will be
    // used to determine whether the current thread holds the global VM
    // state lock or not (if not, operations are queued).
    static void initThreading(VmLock* vmLock);

    // A base class for all AndroidPipe services, which is in charge
    // of creating new instances when a guest client connects to the
    // service.
    class Service {
    public:
        // Explicit constructor.
        explicit Service(const char* name) : mName(name) {}

        // Default destructor.
        virtual ~Service() = default;

        // Return service name.
        const std::string& name() const { return mName; }

        // Create a new pipe instance. This will be called when a guest
        // client connects to the service identified by its registration
        // name (see add() below). |hwPipe| is the hardware-side
        // view of the pipe, and |args| potential arguments. Must
        // return nullptr on error.
        virtual AndroidPipe* create(void* hwPipe, const char* args) = 0;

        // Called once per whole vm save/load operation.
        virtual void preLoad(android::base::Stream* stream) {}
        virtual void postLoad(android::base::Stream* stream) {}
        virtual void preSave(android::base::Stream* stream) {}
        virtual void postSave(android::base::Stream* stream) {}

        // Returns true if loading pipe instances from a stream is
        // supported. If true, the load() method will be called to load
        // every pipe instance state from the stream, if false, the
        // method will never be loaded, and the guest pipe channels will
        // be force-closed instead. The default implementation returns
        // false.
        virtual bool canLoad() const { return false; }

        // Load a pipe instance from input |stream|. Only called if
        // canLoad() returns true. Default implementation returns nullptr
        // to indicate an error loading the instance.
        virtual AndroidPipe* load(void* hwPipe,
                                  const char* args,
                                  android::base::Stream* stream) {
            return nullptr;
        }

        // Register a new |service| instance. After the call, the object
        // is owned by the global service manager, and will be destroyed
        // when resetAll() is called.
        static void add(Service* service);

        // Reset the list of registered services. Useful at the start and
        // end of a unit-test.
        static void resetAll();

    protected:
        // No default constructor.
        Service() = delete;

        std::string mName;
    };

    // Default destructor.
    virtual ~AndroidPipe();

    // Called from the device thread to close the pipe entirely.
    // The pipe management code will never access the instance after this call,
    // which means the method is free to delete it before returning.
    virtual void onGuestClose(PipeCloseReason reason) = 0;

    // Called from the device thread to poll the pipe state. Must return a
    // combination of PIPE_POLL_IN, PIPE_POLL_OUT and PIPE_POLL_HUP.
    virtual unsigned onGuestPoll() const = 0;

    // Called from the device thread when the guest wants to receive data
    // from the pipe. |buffers| points to an array of |numBuffers| descriptors
    // that specify where to copy the data to. Return the number of bytes that
    // were actually transferred, 0 for EOF status, or a negative error
    // value otherwise, including PIPE_ERROR_AGAIN which indicates there
    // is no data yet to read.
    virtual int onGuestRecv(AndroidPipeBuffer* buffers, int numBuffers) = 0;

    // Called from the device thread when the guest wants to send data to
    // the pipe. |buffers| points to an array of |numBuffers| descriptors that
    // specify where to copy the data from. Return the number of bytes that
    // were actually transferred, 0 for EOF status, or a negative error
    // value otherwise, including PIPE_ERROR_AGAIN which indicates the pipe
    // is not ready to receive any data yet.
    virtual int onGuestSend(const AndroidPipeBuffer* buffers,
                            int numBuffers) = 0;

    // Called from the device thread when the guest wants to indicate it
    // wants to be woken up when the set of PIPE_WAKE_XXX event bits in |flags|
    // will occur. The pipe implementation should call the wake() method to
    // signal the guest.
    virtual void onGuestWantWakeOn(int flags) = 0;

    // Called to save the pipe state to a |stream|, i.e. when saving snapshots.
    // Note that this is never called if the service's canLoad() method returns
    // false. TODO(digit): Is this always called from the device thread?
    virtual void onSave(android::base::Stream* stream){};

    // Method used to signal the guest that the events corresponding to the
    // PIPE_WAKE_XXX bits in |flags| occurred. NOTE: This can be called from
    // any thread.
    void signalWake(int flags);

    // Method used to signal the guest that the pipe instance wants to close
    // the pipe. NOTE: This can be called from any thread.
    void closeFromHost();

    // Method for canceling any pending wake() or close() that's scheduled for
    // this pipe.
    void abortPendingOperation();

    // Return the name of the AndroidPipe service.
    const char* name() const {
        return mService ? mService->name().c_str() : "<null>";
    }

    // The following functions are implementation details. They are in the
    // public scope to make the implementation of android_pipe_guest_save()
    // and android_pipe_guest_load() easier. DO NOT CALL THEM DIRECTLY.

    // Save an AndroidPipe instance state to a file |stream|.
    void saveToStream(android::base::Stream* stream);

    // Load an AndroidPipe instance from its saved state from |stream|.
    // |hwPipe| is the hardware-side view of the pipe. On success, return
    // a new instance pointer and sets |*pForceClose| to 0 or 1. A value
    // of 1 means that the guest pipe connection should be forcibly closed
    // because the implementation doesn't allow the state to be saved/loaded
    // (e.g. network pipes).
    static AndroidPipe* loadFromStream(android::base::Stream* stream,
                                       void* hwPipe,
                                       char* pForceClose);

    // A variant of loadFromStream() that is only used to support legacy
    // snapshot format for pipes. On success, sets |*pChannel|, |*pWakes| and
    // |*pClosed|, as well as |*pForceClose|. Only used by the QEMU1 virtual
    // pipe device, and will probably be removed in the future.
    static AndroidPipe* loadFromStreamLegacy(android::base::Stream* stream,
                                             void* hwPipe,
                                             uint64_t* pChannel,
                                             unsigned char* pWakes,
                                             unsigned char* pClosed,
                                             char* pForceClose);

protected:
    // No default constructor.
    AndroidPipe() = delete;

    // Constructor used by derived classes only.
    AndroidPipe(void* hwPipe, Service* service)
        : mHwPipe(hwPipe), mService(service) {}

    void* const mHwPipe = nullptr;
    Service* mService = nullptr;
    std::string mArgs;
};

}  // namespace android
