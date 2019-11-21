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

#include "FenceSync.h"

#include "DispatchTables.h"
#include "FrameBuffer.h"
#include "OpenGLESDispatch/EGLDispatch.h"
#include "RenderThreadInfo.h"
#include "StalePtrRegistry.h"

#include "android/base/containers/Lookup.h"
#include "android/base/files/StreamSerializing.h"
#include "android/base/synchronization/Lock.h"

#include <unordered_set>

FenceSync::FenceSync(bool hasNativeFence,
                     bool destroyWhenSignaled) :
    mDestroyWhenSignaled(destroyWhenSignaled) {
    addToRegistry();

    assert(mCount == 1);
    if (hasNativeFence) incRef();

    // assumes that there is a valid + current OpenGL context
    assert(RenderThreadInfo::get());
    assert(RenderThreadInfo::get()->currContext.get());

    mDisplay = FrameBuffer::getFB()->getDisplay();
    mSync = s_egl.eglCreateSyncKHR(mDisplay,
                                   EGL_SYNC_FENCE_KHR,
                                   NULL);
}

FenceSync::~FenceSync() {
    removeFromRegistry();
}

EGLint FenceSync::wait(uint64_t timeout) {
    incRef();
    EGLint wait_res =
        s_egl.eglClientWaitSyncKHR(mDisplay, mSync,
                                   EGL_SYNC_FLUSH_COMMANDS_BIT_KHR,
                                   timeout);
    decRef();
    return wait_res;
}

void FenceSync::signaledNativeFd() {
    if (!decRef() && mDestroyWhenSignaled) {
        decRef();
    }
    // NOTE: Do not use anything like this construction:
    // decRef();
    // if (mDestroyWhenSignaled) ...
    // If the object was originally constructed with
    // mDestroyWhenSignaled == false,
    // the first decRef() will have destroyed this object,
    // which would make subsequent access to
    // |mDestroyWhenSignaled| corrupt memory immediately.
    // Please keep this in mind.
}

void FenceSync::destroy() {
    s_egl.eglDestroySyncKHR(mDisplay, mSync);
}

// Snapshots for FenceSync//////////////////////////////////////////////////////
// It's possible, though it does not happen often, that a fence
// can be created but not yet waited on by the guest, which
// needs careful handling:
//
// 1. Avoid manipulating garbage memory on snapshot restore;
// rcCreateSyncKHR *creates new fence in valid memory*
// --snapshot--
// rcClientWaitSyncKHR *refers to uninitialized memory*
// rcDestroySyncKHR *refers to uninitialized memory*
// 2. Make rcCreateSyncKHR/rcDestroySyncKHR implementations return
// the "signaled" status if referring to previous snapshot fences. It's
// assumed that the GPU is long done with them.
// 3. Avoid name collisions where a new FenceSync object is created
// that has the same uint64_t casting as a FenceSync object from a previous
// snapshot.

// Maintain a StalePtrRegistry<FenceSync>:
static android::base::LazyInstance<StalePtrRegistry<FenceSync> >
    sFenceRegistry = LAZY_INSTANCE_INIT;

// static
void FenceSync::addToRegistry() {
    sFenceRegistry->addPtr(this);
}

// static
void FenceSync::removeFromRegistry() {
    sFenceRegistry->removePtr(this);
}

// static
void FenceSync::onSave(android::base::Stream* stream) {
    sFenceRegistry->makeCurrentPtrsStale();
    sFenceRegistry->onSave(stream);
}

// static
void FenceSync::onLoad(android::base::Stream* stream) {
    sFenceRegistry->onLoad(stream);
}

// static
FenceSync* FenceSync::getFromHandle(uint64_t handle) {
    return sFenceRegistry->getPtr(handle);
}
