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

#include "SyncThread.h"

#include "DispatchTables.h"
#include "FrameBuffer.h"
#include "RenderThreadInfo.h"
#include "StalePtrRegistry.h"

#include "OpenGLESDispatch/EGLDispatch.h"

#include "android/utils/debug.h"
#include "emugl/common/crash_reporter.h"
#include "emugl/common/sync_device.h"

#include <sys/time.h>

#define DEBUG 0

#if DEBUG

static uint64_t curr_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_usec / 1000 + tv.tv_sec * 1000;
}

#define DPRINT(fmt, ...) do { \
    if (!VERBOSE_CHECK(syncthreads)) VERBOSE_ENABLE(syncthreads); \
    VERBOSE_TID_FUNCTION_DPRINT(syncthreads, "@ time=%llu: " fmt, curr_ms(), ##__VA_ARGS__); \
} while(0)

#else

#define DPRINT(...)

#endif

static const uint32_t kTimelineInterval = 1;
static const uint64_t kDefaultTimeoutNsecs = 5ULL * 1000ULL * 1000ULL * 1000ULL;

SyncThread::SyncThread(EGLContext parentContext) :
//    osUtils::Thread(android::base::ThreadFlags::MaskSignals, 512 * 1024),
    mDisplay(EGL_NO_DISPLAY),
    mContext(0), mSurf(0) {
    if (parentContext == EGL_NO_CONTEXT) {
        DPRINT("Warning: attempted to start a SyncThread "
               "with EGL_NO_CONTEXT as parent context!");
    }

    FrameBuffer *fb = FrameBuffer::getFB();
    mDisplay = fb->getDisplay();

    this->start();

    initSyncContext();
}

void SyncThread::triggerWait(FenceSync* fenceSync,
                             uint64_t timeline) {
    DPRINT("fenceSyncInfo=0x%llx timeline=0x%lx ...",
            fenceSync, timeline);
    SyncThreadCmd to_send;
    to_send.opCode = SYNC_THREAD_WAIT;
    to_send.fenceSync = fenceSync;
    to_send.timeline = timeline;
    DPRINT("opcode=%u", to_send.opCode);
    sendAsync(to_send);
    DPRINT("exit");
}

void SyncThread::cleanup() {
    DPRINT("enter");
    SyncThreadCmd to_send;
    to_send.opCode = SYNC_THREAD_EXIT;
    sendAndWaitForResult(to_send);
    DPRINT("exit");
}

// Private methods below////////////////////////////////////////////////////////

void SyncThread::initSyncContext() {
    DPRINT("enter");
    SyncThreadCmd to_send;
    to_send.opCode = SYNC_THREAD_INIT;
    sendAndWaitForResult(to_send);
    DPRINT("exit");
}

int SyncThread::Main() {
    DPRINT("in sync thread");

    bool exiting = false;
    uint32_t num_iter = 0;

    while (!exiting) {
        SyncThreadCmd cmd = {};

        DPRINT("waiting to receive command");
        mInput.receive(&cmd);
        num_iter++;

        DPRINT("sync thread @%p num iter: %u", this, num_iter);

        bool need_reply = cmd.needReply;
        int result = doSyncThreadCmd(&cmd);

        if (need_reply) {
            DPRINT("sending result");
            mOutput.send(result);
            DPRINT("sent result");
        }

        if (cmd.opCode == SYNC_THREAD_EXIT) exiting = true;
    }

    DPRINT("exited sync thread");
    return 0;
}

int SyncThread::sendAndWaitForResult(SyncThreadCmd& cmd) {
    DPRINT("send with opcode=%d", cmd.opCode);
    cmd.needReply = true;
    mInput.send(cmd);
    int result = -1;
    mOutput.receive(&result);
    DPRINT("result=%d", result);
    return result;
}

void SyncThread::sendAsync(SyncThreadCmd& cmd) {
    DPRINT("send with opcode=%u fenceSyncInfo=0x%llx",
           cmd.opCode, cmd.fenceSync);
    cmd.needReply = false;
    mInput.send(cmd);
}

void SyncThread::doSyncContextInit() {
    // |mTLS| is cleaned up on doExit(), which is called
    // when RenderThreads exit.
    // Note that this will make the subsequent
    // FrameBuffer::*** calls use |mTLS| as thread local storage,
    // because the semantics of the |RenderThreadInfo| constructor
    // are to both create the object _and_ to set it to
    // the thread local copy.
    mTLS = new RenderThreadInfo();

    DPRINT("enter");
    FrameBuffer::getFB()->createTrivialContext(0, // no share context
                                               &this->mContext,
                                               &this->mSurf);
    FrameBuffer::getFB()->bindContext(mContext, mSurf, mSurf);
    DPRINT("exit");
}

void SyncThread::doSyncWait(SyncThreadCmd* cmd) {
    DPRINT("enter");

    FenceSync* fenceSync =
        FenceSync::getFromHandle((uint64_t)(uintptr_t)cmd->fenceSync);

    if (!fenceSync) {
        emugl_sync_timeline_inc(cmd->timeline, kTimelineInterval);
        return;
    }

    EGLint wait_result = 0x0;

    DPRINT("wait on sync obj: %p", cmd->fenceSync);
    wait_result = cmd->fenceSync->wait(kDefaultTimeoutNsecs);

    DPRINT("done waiting, with wait result=0x%x. "
           "increment timeline (and signal fence)",
           wait_result);

    if (wait_result != EGL_CONDITION_SATISFIED_KHR) {
        DPRINT("error: eglClientWaitSync abnormal exit 0x%x\n",
               wait_result);
    }

    DPRINT("issue timeline increment");

    // We always unconditionally increment timeline at this point, even
    // if the call to eglClientWaitSync returned abnormally.
    // There are three cases to consider:
    // - EGL_CONDITION_SATISFIED_KHR: either the sync object is already
    //   signaled and we need to increment this timeline immediately, or
    //   we have waited until the object is signaled, and then
    //   we increment the timeline.
    // - EGL_TIMEOUT_EXPIRED_KHR: the fence command we put in earlier
    //   in the OpenGL stream is not actually ever signaled, and we
    //   end up blocking in the above eglClientWaitSyncKHR call until
    //   our timeout runs out. In this case, provided we have waited
    //   for |kDefaultTimeoutNsecs|, the guest will have received all
    //   relevant error messages about fence fd's not being signaled
    //   in time, so we are properly emulating bad behavior even if
    //   we now increment the timeline.
    // - EGL_FALSE (error): chances are, the underlying EGL implementation
    //   on the host doesn't actually support fence objects. In this case,
    //   we should fail safe: 1) It must be only very old or faulty
    //   graphics drivers / GPU's that don't support fence objects.
    //   2) The consequences of signaling too early are generally, out of
    //   order frames and scrambled textures in some apps. But, not
    //   incrementing the timeline means that the app's rendering freezes.
    //   So, despite the faulty GPU driver, not incrementing is too heavyweight a response.

    emugl_sync_timeline_inc(cmd->timeline, kTimelineInterval);
    cmd->fenceSync->signaledNativeFd();

    DPRINT("done timeline increment");

    DPRINT("exit");
}

void SyncThread::doExit() {
    // This sequence parallels the exit sequence
    // in RenderThread.
    FrameBuffer::getFB()->bindContext(0, 0, 0);
    FrameBuffer::getFB()->DestroyWindowSurface(mSurf);
    FrameBuffer::getFB()->drainWindowSurface();
    FrameBuffer::getFB()->drainRenderContext();
    delete mTLS;
}

int SyncThread::doSyncThreadCmd(SyncThreadCmd* cmd) {
    int result = 0;
    switch (cmd->opCode) {
    case SYNC_THREAD_INIT:
        DPRINT("exec SYNC_THREAD_INIT");
        doSyncContextInit();
        break;
    case SYNC_THREAD_WAIT:
        DPRINT("exec SYNC_THREAD_WAIT");
        doSyncWait(cmd);
        break;
    case SYNC_THREAD_EXIT:
        DPRINT("exec SYNC_THREAD_EXIT");
        doExit();
        break;
    }
    return result;
}

// Interface for libOpenglRender TLS

/* static */
SyncThread* SyncThread::getSyncThread() {
    RenderThreadInfo* tInfo = RenderThreadInfo::get();

    if (!tInfo->syncThread.get()) {
        DPRINT("starting a sync thread for render thread info=%p", tInfo);
        tInfo->createSyncThread();
    }

    return tInfo->syncThread.get();
}

/* static */
void SyncThread::destroySyncThread() {
    RenderThreadInfo* tInfo = RenderThreadInfo::get();

    DPRINT("exiting a sync thread for render thread info=%p.", tInfo);

    if (!tInfo->syncThread) return;

    tInfo->syncThread->cleanup();
    intptr_t exitStatus;
    tInfo->syncThread->wait(&exitStatus);
    tInfo->destroySyncThread();
}

/* static */
SyncThread* SyncThread::getFromHandle(uint64_t alias) {
    return RenderThreadInfo::getSyncThreadRegistry()->getPtr(
            alias,
            reinterpret_cast<SyncThread*>(alias) /* return cast as default */,
            true /* remove if getting stale ptr */);
}
