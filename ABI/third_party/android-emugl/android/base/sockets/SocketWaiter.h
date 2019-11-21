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

#include <stdint.h>

namespace android {
namespace base {

// A SocketWaiter is an abstraction that allows one to wait for i/o
// events on a set of socket file descriptors for a certain amount
// of time. Usage is:
//
//  1/ Create the waiter instance:
//
//       SocketWaiter* waiter = SocketWaiter::create();
//
//  2/ Call the update() method to indicate which I/O events you want
//     to listen to for a given file descriptor |fd|:
//
//       waiter->update(fd1, SocketWaiter::kEventRead);
//       waiter->update(fd2, SocketWaiter::kEventWrite);
//
//  3/ Call the wait() method to wait for i/o events on the registered
//     socket file descriptors.
//
//       int ret = waiter->wait(timeout_ms);
//
//  4/ After wait() returns, call nextPendingFd() to get the list of
//     file descriptors that have pending i/o events, and the corresponding
//     event mask. The function returns -1 once you reach past the list:
//
//       for (;;) {
//          unsigned events = 0;
//          int fd = waiter->nextPendingFd(&events);
//          if (fd < 0) {
//            break;
//          }
//          // Handle |events| on socket |fd|.
//       }
//
//  5/ Which file descriptors are being watched is recorded by the waiter
//     between several calls to wait(). You can use reset() to clear all
//     of them at once, or call update() with 0 as the second parameter to
//     indicate you no longer want to listen to events on a given
//     descriptor, as in:
//
//        waiter->update(fd2, 0);
//
class SocketWaiter {
public:
    enum Event {
        kEventRead = (1U << 0),
        kEventWrite = (1U << 1),
    };

    // Create new SocketWaiter instance.
    static SocketWaiter* create();

    // Destroy the instance.
    virtual ~SocketWaiter() {}

    // Reset the waiter, i.e. equivalent to calling update(fd, 0) on
    // all previously registered socket descriptors.
    virtual void reset() = 0;

    // Return the current wanted event bitmask for a given |fd|.
    virtual unsigned wantedEventsFor(int fd) const = 0;

    // Return the pending event bitmask for a given |fd|. Only use this
    // after calling wait().
    virtual unsigned pendingEventsFor(int fd) const = 0;

    // Return true iff there are registered socket descriptors.
    virtual bool hasFds() const = 0;

    // Tell the waiter to look for i/o events on socket descriptor |fd|.
    // |events| is a bitmask containing kEventRead, kEventWrite or both.
    virtual void update(int fd, unsigned events) = 0;

    // Wait at most |timeout_ms| milli-seconds for i/o events to occur
    // on registered socket descriptors. If |timeout_ms| is 0, return
    // immediately after polling the descriptors. If it is INT64_MAX,
    // then wait indefinitely.
    //
    // Return the number of descriptors that have i/o events.
    // In case of timeout, return 0 and sets errno to ETIMEDOUT.
    // In case of error, return -1/errno.
    //
    // This function loops around EINTR as a convenience.
    virtual int wait(int64_t timeout_ms) = 0;

    // Get the next pending socket descriptor and associated i/o event mask.
    // Return the next socket descriptor, or -1 when the end of the list
    // is reached. On success, sets |*fdEvents| to the corresponding
    // event mask.
    virtual int nextPendingFd(unsigned* fdEvents) = 0;

protected:
    SocketWaiter() {}

    DISALLOW_COPY_AND_ASSIGN(SocketWaiter);
};

}  // namespace base
}  // namespace android
