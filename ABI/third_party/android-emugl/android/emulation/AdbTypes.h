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

#include "android/base/sockets/ScopedSocket.h"

#include <memory>

namespace android {
namespace emulation {

// Interfaces used to implement the communication channel between the
// host ADB Server and the guest adbd daemon, provided by the emulator.
//
// Two objects interact to implement the proxy:
//
// - The AdbHostAgent listens for connections from the server, and passes
//   connection sockets to the AdbGuestAgent when they happen.
//
// - The AdbGuestAgent handles all guest ADB pipe connections, and tells
//   the AdbHostAgent when to accept, or not accept connections, as well as
//   when to notify the server that a new guest is waiting for it.

// AdbGuestAgent is an abstract interface to an object that models
// all guest ADB pipe connections.
struct AdbGuestAgent {
    // Ensure destructor is virtual.
    virtual ~AdbGuestAgent() = default;

    // Called when the host ADB server connects. This passes ownership of
    // |socket| to the AdbGuestAgent instance. This should only happen after
    // startListening() was called on the AdbHostAgent.
    virtual void onHostConnection(android::base::ScopedSocket&& socket) = 0;
};

// AdbHostAgent is an abstract class to the code that actually
// manages the host ADB server port and multiple ADB pipes.
struct AdbHostAgent {
    // Ensure destructor is virtual.
    virtual ~AdbHostAgent() = default;

    // Called to tell the AdbHostAgent to accept one connection from the
    // ADB server. When this happens, onHostConnection() will be called,
    // and the AdbGuestAgent will have to call startListening() again
    // to allow another connection.
    virtual void startListening() = 0;

    // Tell the AdbHostAgent to stop accepting connections from the ADB server.
    // This can happen after startListening() if for some reason the guest
    // destroys the current guest connection that's waiting for it.
    virtual void stopListening() = 0;

    // Tell the ADB server that a new guest is waiting for connections.
    // This is important in the case where the ADB Server was restarted and
    // the current emulator instance is listening on one a port that is not
    // one of the 16 auto-scanned ports starting from 5555.
    virtual void notifyServer() = 0;
};

}  // namespace emulation
}  // namespace android
