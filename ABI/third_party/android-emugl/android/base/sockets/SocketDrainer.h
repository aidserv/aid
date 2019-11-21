// Copyright 2014 The Android Open Source Project
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

#include "android/base/Compiler.h"
#include "android/base/async/Looper.h"

// The following functions are used to manage graceful close of sockets
// as described by "The ultimate SO_LINGER page" [1], which requires
// several steps to do properly.
//
// To use it, simply create a socketDrainer instance after creating a
// Looper, as in:
//
//      SocketDrainer  socketDrainer(looper);
//
// When a socket needs to be gracefully closed, call the following:
//
//       socketDrainer.drainAndClose(socketFd);
//
// When the drainer is destroyed (e.g. failing out of scope, or through
// delete), all remaining sockets will be shut down and closed without
// further draining.
//
// [1] http://blog.netherlabs.nl/articles/2009/01/18/the-ultimate-so_linger-page-or-why-is-my-tcp-not-reliable

namespace android {
namespace base {

class SocketDrainerImpl;
class SocketDrainer {
public:
    SocketDrainer(Looper* looper);

    ~SocketDrainer();

    // Add a socket to be drained and closed
    void drainAndClose(int socketFd);

    static void drainAndCloseBlocking(int socketFd);

private:
    SocketDrainerImpl*  mSocketDrainerImpl;
};

} // namespace base
} // namespace android
