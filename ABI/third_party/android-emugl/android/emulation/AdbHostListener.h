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

#include "android/base/async/AsyncSocketServer.h"
#include "android/emulation/AdbTypes.h"

#include <memory>
#include <vector>

namespace android {
namespace emulation {

// AdbHostListener implements the AdbHostAgent interface with a concrete
// implementation that uses localhost TCP ports. Its full purpose is
// the following:
//
// - Bind to a localhost (IPv4 and IPv6) TCP port waiting for connections
//   from the ADB Server running on the host.
//
// - Record guest pipe connections from the 'adbd' daemon running in the
//   emulated system, through the addGuest() method.
//
// - When a new guest connection happens, start listening for TCP connections.
//   When one such connection happens, send its socket to the guest connection
//   which will handle network traffic on it, until deleteGuest() is called.
//
// - Notify the ADB Server of new guest connections (i.e. emulator instances
//   that appear under 'adb devices'), through the notifyAdbServer()
//   method, which can be called either explictly, or automatically every
//   time a guest connection is activated by a TCP connection.
//
// Usage is the following:
//
//  - Create new AdbHostListener instance, passing a valid |adbClientPort|
//    parameter to the constructor (e.g. kDefaultAdbClientPort) to ensure
//    that it will notify the host ADB server of new guest connections
//    properly (the default value of 0 is useful for unit-testing).
//
//  - Call the init() method to setup listening on localhost port |adbPort|,
//    this can be un-done with a call to deinit().
//
//  - At runtime, call addGuest() and deleteGuest() to manage guest
//    connections.
//
// NOTE: Everything should happen in the main loop thread.
//
class AdbHostListener : public AdbHostAgent {
public:
    // Constructor takes an option |adbClientPort| parameter specifying
    // the client port that the ADB server is listening on for notifications
    // or new emulator instances. This will be used to call notifyAdbServer()
    // when a guest <-> host connection occurs. A value of 0 means to never
    // notify the server. See kDefaultAdbClientPort too.
    explicit AdbHostListener(int adbClientPort = 0)
            : mAdbClientPort(adbClientPort) {}

    // Return the ADB client port used by this listener.
    int clientPort() const { return mAdbClientPort; }

    // Set the AdbGuestAgent to use at runtime. Its onHostConnection()
    // method will be called when the ADB server connects after a call
    // to startListening().
    void setGuestAgent(AdbGuestAgent* guestAgent) { mGuestAgent = guestAgent; }

    // Call this method to initialize bind to TCP localhost
    // port |adbEmulatorPort| (both IPv4 and IPv6). A value of 0 will bind
    // to any random available port. A value of -1 unbinds the current port,
    // if any. Return true on success, false/errno on error.
    bool reset(int adbEmulatorPort);

    // AdbHostAgent method overrides.
    virtual void startListening() override;
    virtual void stopListening() override;
    virtual void notifyServer() override;

    // Return port this host server is currently bound to, or -1 if it is not.
    int port() const { return mServer.get() ? mServer->port() : -1; }

private:
    // Called from the AsyncSocketServer when a new connection from the
    // ADB host server happens. Return true on success, false otherwise.
    bool onHostServerConnection(int port);

    AdbGuestAgent* mGuestAgent = nullptr;
    int mAdbClientPort = -1;
    std::unique_ptr<android::base::AsyncSocketServer> mServer;
};

}  // namespace emulation
}  // namespace android
