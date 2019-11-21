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

#ifndef _EGL_SYNC_H
#define _EGL_SYNC_H

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <inttypes.h>

// EGLSync_t is our driver's internal representation
// of EGLSyncKHR objects.
// The components are:
// a. handle: reference host sync objects
// b. android_native_fence_fd: for ANDROID_native_fence_sync
// when we want to wrap native fence fd's in EGLSyncKHR's
// c. type/status: track status of fence so SyncAttrib works
struct EGLSync_t {
    EGLSync_t(uint64_t handle_in) :
        handle(handle_in), android_native_fence_fd(-1),
        type(EGL_SYNC_FENCE_KHR),
        status(EGL_UNSIGNALED_KHR) { }
    uint64_t handle;
    int android_native_fence_fd;
    EGLint type;
    EGLint status;
};

#endif
