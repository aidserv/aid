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

#include "android/emulation/goldfish_sync.h"
#include "android/emulation/GoldfishSyncCommandQueue.h"

#include "android/base/containers/Lookup.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/synchronization/Lock.h"

#include "android/utils/assert.h"
#include "android/utils/debug.h"
#include "android/utils/system.h"

#include <unordered_map>

using android::base::AutoLock;
using android::base::ConditionVariable;
using android::base::Lock;
using android::GoldfishSyncCommandQueue;

// Commands can be tagged with with unique id's,
// so that for the commands that require a reply
// from the guest, we signal them properly.
static uint64_t sUniqueId = 0;
// When we track command completion, we need to be
// careful about concurrent access.
// |sCommandReplyLock| protects
// |sUniqueId| and |wait_map|, including
// the |CommandWaitInfo| structures within.
static android::base::LazyInstance<Lock> sCommandReplyLock =
    LAZY_INSTANCE_INIT;

uint64_t next_unique_id() {
    AutoLock lock(sCommandReplyLock.get());
    uint64_t res = sUniqueId;
    sUniqueId += 1;
    return res;
}

struct CommandWaitInfo {
    Lock lock; // protects other parts of this struct
    bool done = false;
    ConditionVariable cvDone;
    uint64_t return_value;
};

// |wait_map| keeps track of all the commands in flight
// that require a reply from the guest.
static std::unordered_map<uint64_t, std::unique_ptr<CommandWaitInfo> >
    wait_map;

static CommandWaitInfo* allocWait(uint64_t id) {
    AutoLock lock(sCommandReplyLock.get());
    std::unique_ptr<CommandWaitInfo>& res =
        wait_map[id];
    res.reset(new CommandWaitInfo);
    return res.get();
}

static void freeWait(uint64_t id) {
    AutoLock lock(sCommandReplyLock.get());
    wait_map.erase(id);
}

static GoldfishSyncDeviceInterface* sGoldfishSyncHwFuncs = NULL;

////////////////////////////////////////////////////////////////////////////////
// Goldfish sync device: command send/receive protocol
// To send commands to the virtual device, there are two
// alternatives:
// - |sendCommand|, which just sends the command without waiting
//   for a reply.
// - |sendCommandAndGetResult|, which sends the command and waits
//   for that command to have completed in the guest.

// |sendCommand| is used to send Goldfish sync commands while
// not caring about a reply from the guest. During normal operation,
// we will only use |sendCommand| to send over a |goldfish_sync_timeline_inc|
// call, to signal fence FD's on the guest.
static void sendCommand(uint32_t cmd,
                        uint64_t handle,
                        uint32_t time_arg) {
    GoldfishSyncCommandQueue::hostSignal
        (cmd, handle, time_arg, 0
         // last arg 0 OK because we will not reference it
        );
}

// Receiving commands can be interesting because we do not know when
// the kernel will get to servicing a command we sent from the host.
//
// |receiveCommandResult| is for host->guest goldfish sync commands
// that require a reply from the guest. So far, this is used only
// in the functional tests, as we never issue
// |goldfish_sync_create_timeline| / |goldfish_sync_create_fence|
// from the host directly in normal operation.
//
// This function will be called by the virtual device
// upon receiving a reply from the guest for the host->guest
// commands that require replies.
//
// The implementation is that such commands will use a condition
// variable that waits on the result.
void goldfish_sync_receive_hostcmd_result(uint32_t cmd,
                                          uint64_t handle,
                                          uint32_t time_arg,
                                          uint64_t hostcmd_handle) {
    if (auto elt = android::base::find(wait_map, hostcmd_handle)) {
        CommandWaitInfo* wait_info = elt->get();
        AutoLock lock(wait_info->lock);
        wait_info->return_value = handle;
        wait_info->done = true;
        wait_info->cvDone.broadcast();
    }
}

// |sendCommandAndGetResult| uses |sendCommand| and
// |goldfish_sync_receive_hostcmd_result| for processing
// commands that require replies from the guest.
static uint64_t sendCommandAndGetResult(uint64_t cmd,
                                        uint64_t handle,
                                        uint64_t time_arg,
                                        uint64_t hostcmd_handle) {

    // queue a signal to the device
    GoldfishSyncCommandQueue::hostSignal
        (cmd, handle, time_arg, hostcmd_handle);

    CommandWaitInfo* waitInfo = allocWait(hostcmd_handle);

    uint64_t res;

    {
        AutoLock lock(waitInfo->lock);
        while (!waitInfo->done) {
            waitInfo->cvDone.wait(&waitInfo->lock);
        }

        res = waitInfo->return_value;
    }

    freeWait(hostcmd_handle);

    return res;
}

// Goldfish sync host-side interface implementation/////////////////////////////

uint64_t goldfish_sync_create_timeline() {
    return sendCommandAndGetResult(CMD_CREATE_SYNC_TIMELINE,
                                   0, 0, next_unique_id());
}

int goldfish_sync_create_fence(uint64_t timeline, uint32_t pt) {
    return (int)sendCommandAndGetResult(CMD_CREATE_SYNC_FENCE,
                                        timeline, pt, next_unique_id());
}

void goldfish_sync_timeline_inc(uint64_t timeline, uint32_t howmuch) {
    sendCommand(CMD_SYNC_TIMELINE_INC, timeline, howmuch);
}

void goldfish_sync_destroy_timeline(uint64_t timeline) {
    sendCommand(CMD_DESTROY_SYNC_TIMELINE, timeline, 0);
}

void goldfish_sync_register_trigger_wait(trigger_wait_fn_t f) {
    if (goldfish_sync_device_exists()) {
        sGoldfishSyncHwFuncs->registerTriggerWait(f);
    }
}

bool goldfish_sync_device_exists() {
    // The idea here is that the virtual device should set
    // sGoldfishSyncHwFuncs. If it didn't do that, we take
    // that to mean there is no virtual device.
    return sGoldfishSyncHwFuncs != NULL;
}

void goldfish_sync_set_hw_funcs(GoldfishSyncDeviceInterface* hw_funcs) {
    sGoldfishSyncHwFuncs = hw_funcs;
    GoldfishSyncCommandQueue::setQueueCommand
        (sGoldfishSyncHwFuncs->doHostCommand);
}

