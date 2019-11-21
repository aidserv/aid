/* Copyright (C) 2016 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

#pragma once

#include "android/base/memory/LazyInstance.h"
#include "android/emulation/DeviceContextRunner.h"
#include "android/emulation/goldfish_sync.h"
#include "android/emulation/VmLock.h"

#include <vector>

#define DEBUG 0

#if DEBUG
#define DPRINT(...) do { \
    if (!VERBOSE_CHECK(goldfishsync)) VERBOSE_ENABLE(goldfishsync); \
    VERBOSE_TID_FUNCTION_DPRINT(goldfishsync, __VA_ARGS__); } while(0)
#else
#define DPRINT(...)
#endif

#define ERR(...) do { \
    derror(__VA_ARGS__); \
} while(0)

namespace android {

////////////////////////////////////////////////////////////////////////////////
// GoldfishSyncCommandQueue ensures that commands sent to the Goldfish Sync
// virtual device take place in "device context"; that is, the commands
// are only executed while the main loop has the VM lock. Like PipeWaker
// of AndroidPipe, this class derives from DeviceContextRunner for this
// functionality.
// This class is only used for host->guest commands for goldfish sync device,
// and mainly timeline increment at that. The way to use
// GoldfishSyncCommandQueue in general is to call |hostSignal| with the
// particular details of the host->guest command being issued.
//
// However, make sure that |setQueueCommand| has been called on
// goldifish sync device initialization, which properly hooks up the
// GoldfishSyncCommandQueue with the particular virtual device function
// that actually raises IRQ's, and that |initThreading| has been called
// in a main loop context, preferably in qemu-setup.cpp.

struct GoldfishSyncWakeInfo {
    uint64_t handle;
    uint64_t hostcmd_handle;
    uint32_t cmd;
    uint32_t time_arg;
};

class GoldfishSyncCommandQueue final :
    public DeviceContextRunner<GoldfishSyncWakeInfo> {

public:
    // Like with the PipeWaker for AndroidPipe,
    // we need to process all commands
    // in a context where we hold the VM lock.
    static void initThreading(VmLock* vmLock);

    // Goldfish sync virtual device will give out its own
    // callback for queueing commands to it.
    static void setQueueCommand(queue_device_command_t fx);

    // Main interface for all Goldfish sync device
    // communications.
    static void hostSignal(uint32_t cmd,
                           uint64_t handle,
                           uint32_t time_arg,
                           uint64_t hostcmd_handle);

private:
    static base::LazyInstance<GoldfishSyncCommandQueue> sCommandQueue;
    virtual void performDeviceOperation(const GoldfishSyncWakeInfo& cmd) override;

    queue_device_command_t tellSyncDevice = nullptr;
};

} // namespace android
