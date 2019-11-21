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

#include "android/base/async/ScopedSocketWatch.h"

#include "android/base/sockets/SocketUtils.h"

void android::base::SocketWatchDeleter::operator()(
        android::base::Looper::FdWatch* watch) const {
    int fd = watch->fd();
    if (fd >= 0) {
        socketClose(watch->fd());
    }
    delete watch;
}
