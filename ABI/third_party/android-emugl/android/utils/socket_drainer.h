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

#include "android/utils/compiler.h"
#include "android/utils/looper.h"

ANDROID_BEGIN_HEADER

// The following functions are used to manage graceful close of sockets
// as described by "The ultimate SO_LINGER page" [1], which requires
// several steps to do properly.
//
// This is really a C wrapper around android/base/sockets/SocketDrainer.h
// See the comments in that header for more details.
//
// To use this header from C code, call the following after creating a
// looper:
//
//     socket_drainer_start(looper);
//
// When a socket needs to be gracefully closed, call the following:
//
//     socket_drainer_drain_and_close(socket_fd);
//
// After the looper run completes, call the following:
//
//     socket_drainer_stop();
//
// Note: socket_drainer_drain_and_close() can be called before
// socket_drainer_start() or after socket_drainer_stop(). In those cases
// the socket is simply shutdown and closed without receiving all pending
// data.

void socket_drainer_start(Looper* looper);
void socket_drainer_drain_and_close(int socket_fd);
void socket_drainer_stop();

ANDROID_END_HEADER
