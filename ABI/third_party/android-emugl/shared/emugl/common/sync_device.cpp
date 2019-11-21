/*
* Copyright (C) 2016 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "sync_device.h"

static uint64_t defaultCreateTimeline() { return 0; }

static int defaultCreateFence(uint64_t timeline, uint32_t pt) {
    (void)timeline;
    (void)pt;
    return -1;
}

static void defaultTimelineInc(uint64_t timeline, uint32_t howmuch) {
    (void)timeline;
    (void)howmuch;
    return;
}

static void defaultDestroyTimeline(uint64_t timeline) {
    (void)timeline;
    return;
}

static void defaultRegisterTriggerWait(emugl_sync_trigger_wait_t f) {
    (void)f;
    return;
}

static bool defaultDeviceExists() {
    return false;
}

emugl_sync_create_timeline_t emugl_sync_create_timeline = defaultCreateTimeline;
emugl_sync_create_fence_t emugl_sync_create_fence = defaultCreateFence;
emugl_sync_timeline_inc_t emugl_sync_timeline_inc = defaultTimelineInc;
emugl_sync_destroy_timeline_t emugl_sync_destroy_timeline = defaultDestroyTimeline;
emugl_sync_register_trigger_wait_t emugl_sync_register_trigger_wait = defaultRegisterTriggerWait;
emugl_sync_device_exists_t emugl_sync_device_exists = defaultDeviceExists;

void set_emugl_sync_create_timeline(emugl_sync_create_timeline_t f) {
    emugl_sync_create_timeline = f;
}

void set_emugl_sync_create_fence(emugl_sync_create_fence_t f) {
    emugl_sync_create_fence = f;
}

void set_emugl_sync_timeline_inc(emugl_sync_timeline_inc_t f) {
    emugl_sync_timeline_inc = f;
}

void set_emugl_sync_destroy_timeline(emugl_sync_destroy_timeline_t f) {
    emugl_sync_destroy_timeline = f;
}

void set_emugl_sync_register_trigger_wait(emugl_sync_register_trigger_wait_t f) {
    emugl_sync_register_trigger_wait = f;
}

void set_emugl_sync_device_exists(emugl_sync_device_exists_t f) {
    emugl_sync_device_exists = f;
}
