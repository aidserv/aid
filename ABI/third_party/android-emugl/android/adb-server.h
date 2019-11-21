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

#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

// Notify the ADB server, if any is running on the host, that a new
// emulator instance is
extern void android_adb_server_notify(int port);

// Setup ADB Server listener, expecting connections on TCP |port|.
// Return 0 on success, -1/errno otherwise.
extern int android_adb_server_init(int port);

// Teardown ADB Server listener.
extern void android_adb_server_undo_init(void);

// Register ADB pipe services. This must be performed after a succesfull
// call to adb_server_init() and before adb_server_undo_init(), if any.
extern void android_adb_service_init(void);

ANDROID_END_HEADER
