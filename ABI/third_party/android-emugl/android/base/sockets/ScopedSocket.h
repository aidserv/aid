// Copyright 2014-2017 The Android Open Source Project
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

#include "android/base/sockets/SocketUtils.h"

#include <utility>

namespace android {
namespace base {

class ScopedSocket {
public:
    constexpr ScopedSocket() = default;
    constexpr ScopedSocket(int socket) : mSocket(socket) {}
    ScopedSocket(ScopedSocket&& other) : mSocket(other.release()) {}
    ~ScopedSocket() { close(); }

    ScopedSocket& operator=(ScopedSocket&& other) {
        reset(other.release());
        return *this;
    }

    bool valid() const { return mSocket >= 0; }

    int get() const { return mSocket; }

    void close() {
        if (valid()) {
            socketClose(release());
        }
    }

    int release() {
        int result = mSocket;
        mSocket = -1;
        return result;
    }

    void reset(int socket) {
        close();
        mSocket = socket;
    }

    void swap(ScopedSocket* other) {
        std::swap(mSocket, other->mSocket);
    }

private:
    int mSocket = -1;
};

inline void swap(ScopedSocket& one, ScopedSocket& two) {
    one.swap(&two);
}

}  // namespace base
}  // namespace android
