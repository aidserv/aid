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

#include "android/emulation/android_pipe_device.h"
#include "android/emulation/testing/TestVmLock.h"

#include <memory>

namespace android {

// A class used to implement a test version of the Android pipe virtual device,
// in order to test the code in android/emulation/android_pipe.c.
// Usage is:
//  1) Create a new TestAndroidPipeDevice instance.
//
//  2) For each guest client you want to simulate, create a new
//     TestAndroidPipeDevice::Guest instance, then call its connect()
//     to try to connect with the corresponding host service. In case
//     of success, use its read() and write() methods.
//
class TestAndroidPipeDevice {
public:
    // Default constructor, this registers the device by calling
    // android_pipe_set_hw_funcs()
    TestAndroidPipeDevice();

    // Destructor, this calls android_pipe_set_hw_funcs() to reset
    // android_pipe.c to its previous state.
    ~TestAndroidPipeDevice();

    class Guest {
    public:
        // Create new Guest instance.
        static Guest* create();

        // Destructor, closes the connection, if any.
        virtual ~Guest() {}

        // Try to connect to a named service. |name| can be
        // either <service> or <service>:<args>. Returns 0 on
        // success, or -errno on failure.
        virtual int connect(const char* name) = 0;

        // Try to read data from the host service. Return the
        // number of bytes transfered. 0 for EOF, and -errno for
        // i/o error.
        virtual ssize_t read(void* buffer, size_t len) = 0;

        // Try to write data to the host service. Return the
        // number of bytes transfered. 0 for EOF, and -errno
        // for i/o error.
        virtual ssize_t write(const void* buffer, size_t len) = 0;

        // Force-close the connection from the guest.
        virtual void close() = 0;

        // Poll the current state of the guest connection.
        // Returns a combination of PIPE_POLL_IN and PIPE_POLL_OUT
        // flags.
        virtual unsigned poll() const = 0;

        // Return the AndroidPipe associated with this guest.
        virtual void* getPipe() const = 0;

    protected:
        // Private constructor.
        Guest() {}
    };

private:
    // AndroidPipeHwFuncs callbacks.
    static void closeFromHost(void* hwpipe);
    static void signalWake(void* hwpipe, unsigned wakes);
    static void resetPipe(void* hwpipe, void* internal_pipe);

    static const AndroidPipeHwFuncs sHwFuncs;

    TestVmLock mVmLock;
    const AndroidPipeHwFuncs* mOldHwFuncs;
};

}  // namespace android
