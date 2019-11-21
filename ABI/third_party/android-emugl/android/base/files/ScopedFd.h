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

#include <errno.h>
#include <unistd.h>

namespace android {
namespace base {

// Helper class to hold an integer file descriptor, and have the 'close'
// function called automatically on scope exit, unless the 'release'
// method was called previously.
class ScopedFd {
public:
    // Default constructor will hold an invalid descriptor.
    ScopedFd() : fd_(-1) {}

    // Constructor takes ownership of |fd|.
    explicit ScopedFd(int fd) : fd_(fd) {}

    // Make it movable.
    ScopedFd(ScopedFd&& other) : fd_(other.fd_) {
        other.fd_ = -1;
    }

    // Destructor calls close().
    ~ScopedFd() { close(); }

    // Return the file descriptor value, does _not_ transfer ownership.
    int get() const { return fd_; }

    // Return the file descriptor value, transfers ownership to the caller.
    int release() {
        int fd = fd_;
        fd_ = -1;
        return fd;
    }

    // Return true iff the file descriptor is valid.
    bool valid() const { return fd_ >= 0; }

    // Close the file descriptor (and make the wrapped value invalid).
    void close() {
        if (fd_ != -1) {
            int save_errno = errno;
            ::close(fd_);
            fd_ = -1;
            errno = save_errno;
        }
    }

    // Swap two ScopedFd instances.
    void swap(ScopedFd* other) {
        int fd = fd_;
        fd_ = other->fd_;
        other->fd_ = fd;
    }

private:
    DISALLOW_COPY_AND_ASSIGN(ScopedFd);

    int fd_;
};

}  // namespace base
}  // namespace android
