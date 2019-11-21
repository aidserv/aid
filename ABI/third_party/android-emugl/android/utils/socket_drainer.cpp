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

#include "android/utils/socket_drainer.h"

#include "android/base/Log.h"
#include "android/base/sockets/SocketDrainer.h"
#include "android/base/sockets/SocketUtils.h"

#include <stddef.h>

using namespace android::base;

namespace {

typedef ::Looper CLooper;
typedef ::android::base::Looper BaseLooper;

android::base::SocketDrainer *s_socket_drainer = NULL;

}  // namespace

void socket_drainer_start(CLooper* looper) {
    if (!looper) {
        return;
    }
    if (!s_socket_drainer) {
        s_socket_drainer = new SocketDrainer(
                reinterpret_cast<BaseLooper*>(looper));
    }
}

void socket_drainer_drain_and_close(int socketFd) {
    if (socketFd < 0) {
        return;
    }
    socketSetNonBlocking(socketFd);
    if (s_socket_drainer) {
        s_socket_drainer->drainAndClose(socketFd);
    } else {
        SocketDrainer::drainAndCloseBlocking(socketFd);
    }
}

void socket_drainer_stop() {
    if (s_socket_drainer) {
        delete s_socket_drainer;
        s_socket_drainer = 0;
    }
}
