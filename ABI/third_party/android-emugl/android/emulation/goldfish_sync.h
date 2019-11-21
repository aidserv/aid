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

#include "android/utils/compiler.h"

#include <inttypes.h>
#include <stdbool.h>

ANDROID_BEGIN_HEADER

// GOLDFISH SYNC DEVICE
// The Goldfish sync driver is designed to provide a interface
// between the underlying host's sync device and the kernel's
// sw_sync.
// The purpose of the device/driver is to enable lightweight
// creation and signaling of timelines and fences
// in order to synchronize the guest with host-side graphics events.

// Each time the interrupt trips, the driver
// may perform a sw_sync operation.

// The operations are:

// Ready signal - used to mark when irq should lower
#define CMD_SYNC_READY            0

// Create a new timeline. writes timeline handle
#define CMD_CREATE_SYNC_TIMELINE  1

// Create a fence object. reads timeline handle and time argument.
// Writes fence fd to the SYNC_REG_HANDLE register.
#define CMD_CREATE_SYNC_FENCE     2

// Increments timeline. reads timeline handle and time argument
#define CMD_SYNC_TIMELINE_INC     3

// Destroys a timeline. reads timeline handle
#define CMD_DESTROY_SYNC_TIMELINE 4

// Starts a wait on the host with
// the given glsync object and sync thread handle.
#define CMD_TRIGGER_HOST_WAIT     5

// The register layout is:

#define SYNC_REG_BATCH_COMMAND                0x00 // host->guest batch commands
#define SYNC_REG_BATCH_GUESTCOMMAND           0x04 // guest->host batch commands
#define SYNC_REG_BATCH_COMMAND_ADDR           0x08 // communicate physical address of host->guest batch commands
#define SYNC_REG_BATCH_COMMAND_ADDR_HIGH      0x0c // 64-bit part
#define SYNC_REG_BATCH_GUESTCOMMAND_ADDR      0x10 // communicate physical address of guest->host commands
#define SYNC_REG_BATCH_GUESTCOMMAND_ADDR_HIGH 0x14 // 64-bit part
#define SYNC_REG_INIT                         0x18 // to signal that the device has been detected by the kernel

#define GUEST_TRIGGERED(cmd) (cmd == CMD_SYNC_READY || \
                              cmd == CMD_TRIGGER_HOST_WAIT)

typedef void (*trigger_wait_fn_t)(uint64_t, uint64_t, uint64_t);

// The commands below operate on Android sync "timelines" and
// "fences". The basic concept is as follows.
// - Timeline and fences work together as a state to determine
//   which events have completed. The timeline abstraction
//   is used to represent a monotonic ordering of events,
//   while fences represent levels of progress along
//   timelines. By having different threads wait on fences
//   until their associated timelines make the required progress,
//   we can synchronize multiple threads within a process
//   and with binder, across processes.
// - What follows is an abstract definition of that concept
//   that does not match implementation exactly, but is
//   sufficient for our purposes:
// - A timeline is a represnted by a single uint32_t, known as
//   its "value". Over time, the value may increase.
//   timeline : value
// - Let "timeline-id" be a uint64_t that uniquely identifies each
//   timeline.
//   A fence is an immutable map from timeline-id's to values:
//   fence : map(timeline-id -> value). We will refer to the
//   "timeline-id-values" of a fence to be the set of all pairs
//   (timeline-id, value) that comprise the mapping.
// - Let "fence-id" be an int that uniquely identifies each fence.
//   This is also known as a "fence fd".
//   The important state is represented by a pair of maps:
//   (T = map(timeline-id -> timeline), F = map(fence-id -> fence)).
//   This state may change over time, with timelines and fences
//   being added or removed.
//   For any state, we care primarily about the "signaled" status
//   of the fences within.
//   - Each fence in the map from fence-id's to fences is
//     signaled if:
//     - For all timeline-id-values = (timeline-id, value)
//       of that fence, T[timeline-id] >= value.
//       Or, timeline-id does not exist in T's keys.
//     - Otherwise, it is considered "unsignaled."
// The functions below operate with the above abstraction in mind.
// They all mutate a particular collection of fences and timelines.
// We can think of them as implicitly taking a state (T, F)
// as input and returning a new one (T', F') that is possibly updated,
// according to the changes described. The new collection then
// serves as the true abstract state for all further operations.

// |goldfish_sync_create_timeline| creates a new timeline
// with value 0. The timeline id is returned.
uint64_t goldfish_sync_create_timeline();

// |goldfish_sync_create_fence| creates a fence
// that has a single timeline-id-value pair as its map.
// The first two arguments comprise the pair.
// Returns a unique identifier for the fence.
// Note that this implementation only allows the creation of
// fences associated with a single timeline-id-value pair.
int goldfish_sync_create_fence(uint64_t timeline, uint32_t pt);

// |goldfish_sync_timeline_inc| advances the value component
// of a given timeline by |howmuch|; that is, if
// (t, v) is the timeline-id and value of a timeline before this call,
// (t, v + |howmuch|) is the value after.
// Thus, this may end up changing some fence objects to signaled state.
void goldfish_sync_timeline_inc(uint64_t timeline, uint32_t howmuch);

// |goldfish_sync_destroy_timeline| removes the key |timeline|
// from the global timeline map.
// Any fence objects whose only timeline-id-value (t, v) is such that
// t == |timeline| would be considered signaled.
// If there are any other timeline-id-values, the fence may still be
// unsignaled. We would need the other timelines referenced by this fence
// to have reached the target value of this fence, fence[timeline].
void goldfish_sync_destroy_timeline(uint64_t timeline);

// Registering callbacks for goldfish sync ops
// Currently, only trigger_wait is supported.
void goldfish_sync_register_trigger_wait(trigger_wait_fn_t trigger_fn);

// If the virtual device doesn't actually exist (e.g., when
// using QEMU1), query that using this function:
bool goldfish_sync_device_exists();

// Function types for sending commands to the virtual device.
typedef void (*queue_device_command_t)
    (uint32_t cmd, uint64_t handle, uint32_t time_arg,
     uint64_t hostcmd_handle);

typedef struct GoldfishSyncDeviceInterface {
    // Callback for all communications with virtual device
    queue_device_command_t doHostCommand;

    // Callbacks to register other callbacks for triggering
    // OpenGL waits from the guest
    void (*registerTriggerWait)(trigger_wait_fn_t);
} GoldfishSyncDeviceInterface;

// The virtual device will call |goldfish_sync_set_hw_funcs|
// to connect up to the AndroidEmu part.
extern void
goldfish_sync_set_hw_funcs(GoldfishSyncDeviceInterface* hw_funcs);

// The virtual device calls this |goldfish_sync_receive_hostcmd_result|
// when it receives a reply for host->guest commands that support
// that protocol.
extern void
goldfish_sync_receive_hostcmd_result(uint32_t cmd,
                                     uint64_t handle,
                                     uint32_t time_arg,
                                     uint64_t hostcmd_handle);
ANDROID_END_HEADER
