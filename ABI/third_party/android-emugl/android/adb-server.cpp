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

#include "android/adb-server.h"

#include "android/base/files/StdioStream.h"
#include "android/base/files/Stream.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/system/System.h"
#include "android/emulation/AdbDebugPipe.h"
#include "android/emulation/AdbGuestPipe.h"
#include "android/emulation/AdbHostListener.h"
#include "android/emulation/AdbHostServer.h"
#include "android/emulation/AndroidPipe.h"
#include "android/utils/debug.h"

#include <memory>
#include <string>

#include <stdio.h>
#include <stdlib.h>

namespace {

using android::emulation::AdbGuestPipe;
using android::emulation::AdbHostListener;
using android::emulation::AdbHostServer;
using android::AndroidPipe;

// Global variables used here.
struct Globals {
    Globals() : hostListener(AdbHostServer::getClientPort()) {}

    void registerServices() {
        // Register adb pipe service.
        auto service = new AdbGuestPipe::Service(&hostListener);
        hostListener.setGuestAgent(service);
        AndroidPipe::Service::add(service);

        // Register adb-debug pipe service.
        android::base::Stream* debugStream = nullptr;
        if (VERBOSE_CHECK(adb)) {
            debugStream = new android::base::StdioStream(stderr);
        }
        android::AndroidPipe::Service::add(
                new android::emulation::AdbDebugPipe::Service(debugStream));
    }

    AdbHostListener hostListener;
};

android::base::LazyInstance<Globals> sGlobals = LAZY_INSTANCE_INIT;

}  // namespace

void android_adb_server_notify(int port) {
    auto globals = sGlobals.ptr();
    globals->hostListener.reset(port);
    globals->hostListener.notifyServer();
}

int android_adb_server_init(int port) {
    auto globals = sGlobals.ptr();
    if (!globals->hostListener.reset(port)) {
        return -1;
    }
    return 0;
}

void android_adb_server_undo_init(void) {
    sGlobals->hostListener.reset(-1);
}

void android_adb_service_init(void) {
    // Register adb pipe service.
    sGlobals->registerServices();
}
