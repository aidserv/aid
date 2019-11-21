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

#include <memory>
#include <functional>

namespace android {
namespace base {

class Looper;

class AsyncSocketServer {
public:
    // Type of a function that is called when a new client connection
    // occurs. The parameter is a socket descriptor. Note that stopListening()
    // will have been called before the callback. On success, return true
    // and pass ownership of the socket to the callback. On failure, return
    // false (and the caller will close the socket then call startListening()).
    using ConnectCallback = std::function<bool(int)>;

    // Destructor.
    virtual ~AsyncSocketServer() = default;

    // Return the port that this server is bound to. This is useful if
    // the |port| parameter of functions like createTcpLoopbackServer()
    // was 0, which meant the system found a random free port which can
    // be reported by this function.
    virtual int port() const = 0;

    // Start listening for new connections.
    // Note that by default, a new instance doesn't listen for connections.
    // Moreover, the server will stop listening before invoking the callback
    // on a new connection. User code must call startListening() explicitly
    // to allow more connections to proceed.
    virtual void startListening() = 0;

    // Stop listening for new connections.
    virtual void stopListening() = 0;

    // Values used to indicate which loopback interfaces to listen to.
    // kNone means no listening on any interface.
    // kIPv4 means listening on 127.0.0.1.
    // kIPv6 means listening on ::1
    // kIPv4Optional means that binding to 127.0.0.1 is allowed to fail.
    // kIPv6Optional means that binding to ::1 is allowed to fail.
    // kIPv4AndIPv6 means that the server should bind to both interfaces
    // at the same time.
    // kIPv4AndOptionalIPv6 means that the server must bind to 127.0.0.1 and
    // is allowed to fail to bind to the corresponding ::1 port.
    enum LoopbackMode {
        kNone = 0,
        kIPv4 = (1 << 0),
        kIPv6 = (1 << 1),
        kIPv4Optional = (1 << 2),
        kIPv6Optional = (1 << 3),
        kIPv4AndIPv6 = kIPv4 | kIPv6,
        kIPv4AndOptionalIPv6 = kIPv4 | kIPv6 | kIPv6Optional,
    };

    // Return the set of interfaces the server is listening on.
    // Possible values are kNone, kIPv4, kIPv6 and kIPv4AndIPv6
    virtual LoopbackMode getListenMode() const = 0;

    // Create a new instance that binds to a TCP loopback |port| and will
    // call |connectCallback| when a new host connection occurs. |mode|
    // determines on which interfaces to listen to and defaults to
    // kIPv4AndIPv6, while |looper| can be used to specify a Looper instance
    // (otherwise the thread's current looper will be used).
    // Return an empty std::unique_ptr<> on error.
    static std::unique_ptr<AsyncSocketServer> createTcpLoopbackServer(
            int port,
            ConnectCallback connectCallback,
            LoopbackMode mode = kIPv4AndIPv6,
            Looper* looper = nullptr);

protected:
    // No default constructor, use one of the static createXXX() method to
    // create a new instance.
    AsyncSocketServer() = default;
};

}  // namespace base
}  // namespace android
