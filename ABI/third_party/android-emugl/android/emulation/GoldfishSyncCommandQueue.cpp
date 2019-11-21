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

#include "android/emulation/GoldfishSyncCommandQueue.h"

#include <memory>
#include <string>
#include <vector>

namespace android {

base::LazyInstance<GoldfishSyncCommandQueue>
GoldfishSyncCommandQueue::sCommandQueue = LAZY_INSTANCE_INIT;

// static
void GoldfishSyncCommandQueue::initThreading(VmLock* vmLock) {
    sCommandQueue.ptr()->init(vmLock);
}

// static
void GoldfishSyncCommandQueue::setQueueCommand(queue_device_command_t fx) {
    GoldfishSyncCommandQueue* cmdQueue = sCommandQueue.ptr();
    cmdQueue->tellSyncDevice = fx;
}

// static
void GoldfishSyncCommandQueue::hostSignal(uint32_t cmd,
                                          uint64_t handle,
                                          uint32_t time_arg,
                                          uint64_t hostcmd_handle) {

    GoldfishSyncCommandQueue* queue = sCommandQueue.ptr();

    GoldfishSyncWakeInfo sync_data;
    sync_data.cmd = cmd;
    sync_data.handle = handle;
    sync_data.time_arg = time_arg;
    sync_data.hostcmd_handle = hostcmd_handle;

    queue->queueDeviceOperation(sync_data);
}

void GoldfishSyncCommandQueue::performDeviceOperation
    (const GoldfishSyncWakeInfo& wakeInfo) {
    tellSyncDevice(wakeInfo.cmd,
                   wakeInfo.handle,
                   wakeInfo.time_arg,
                   wakeInfo.hostcmd_handle);
}

} // namespace android
