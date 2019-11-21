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

#pragma once

#include "FenceSync.h"

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "android/base/synchronization/Lock.h"
#include "android/base/synchronization/MessageChannel.h"

// #include "emugl/common/thread.h"
#include "osThread.h"

// SyncThread///////////////////////////////////////////////////////////////////
// The purpose of SyncThread is to track sync device timelines and give out +
// signal FD's that correspond to the completion of host-side GL fence commands.

// We communicate with the sync thread in 3 ways:
enum SyncThreadOpCode {
    // Nonblocking command to initialize sync thread's contents,
    // such as the EGL context for sync operations
    SYNC_THREAD_INIT = 0,
    // Nonblocking command to wait on a given FenceSync object
    // and timeline handle.
    // A fence FD object in the guest is signaled.
    SYNC_THREAD_WAIT = 1,
    // Blocking command to clean up and exit the sync thread.
    SYNC_THREAD_EXIT = 2
};

struct SyncThreadCmd {
    SyncThreadOpCode opCode = SYNC_THREAD_INIT;
    FenceSync* fenceSync = nullptr;
    uint64_t timeline = 0;
    bool needReply = false;
};

struct RenderThreadInfo;
class SyncThread : public osUtils::Thread {
public:
    // - constructor: start up the sync thread for a given context.
    // The initialization of the sync thread is nonblocking.
    // - Triggers a |SyncThreadCmd| with op code |SYNC_THREAD_INIT|
    SyncThread(EGLContext context);
    // |triggerWait|: async wait with a given FenceSync object.
    // We use the wait() method to do a eglClientWaitSyncKHR.
    // After wait is over, the timeline will be incremented,
    // which should signal the guest-side fence FD.
    // This method is how the goldfish sync virtual device
    // knows when to increment timelines / signal native fence FD's.
    void triggerWait(FenceSync* fenceSync,
                     uint64_t timeline);
    // |cleanup|: for use with destructors and other cleanup functions.
    // it destroys the sync context and exits the sync thread.
    // This is blocking; after this function returns, we're sure
    // the sync thread is gone.
    // - Triggers a |SyncThreadCmd| with op code |SYNC_THREAD_EXIT|
    void cleanup();

    // We assume the invariant that sync threads correspond 1:1
    // with render threads.
    // Therefore, we create and destroy sync threads using
    // only the two functions below, |getSyncThread| and |destroySyncThread|.
    // - |getSyncThread| looks at render thread's TLS for a sync thread.
    //   If there isn't one, a sync thread is created and initialized.
    // - |destroySyncThread| cleans up and deletes the sync thread,
    //   if there is one started already.
    //   It is meant to be called whenever render threads exit or
    //   EGL contexts are destroyed.
    static SyncThread* getSyncThread();
    static void destroySyncThread();

    // Safe way to get SyncThread*'s across snapshots.
    static SyncThread* getFromHandle(uint64_t handle);

private:
    // |initSyncContext| creates an EGL context expressly for calling
    // eglClientWaitSyncKHR in the processing caused by |triggerWait|.
    // This is used by the constructor only. It is non-blocking.
    // - Triggers a |SyncThreadCmd| with op code |SYNC_THREAD_INIT|
    void initSyncContext();

    // Thread function executing all sync commands.
    // It listens for |SyncThreadCmd| objects off the message channel
    // |mInput|, and runs them serially.
    // virtual intptr_t main() override final;
    virtual int Main() override final;
    static const size_t kSyncThreadChannelCapacity = 256;
    android::base::MessageChannel<SyncThreadCmd, kSyncThreadChannelCapacity> mInput;

    // |mOutput| holds result of cmds in case of blocking commands
    // that require return results.
    android::base::MessageChannel<GLint, kSyncThreadChannelCapacity> mOutput;

    // These two functions are used to communicate with the sync thread
    // from another thread:
    // - |sendAndWaitForResult| issues |cmd| to the sync thread,
    //   and blocks until it receives the result of the command.
    // - |sendAsync| issues |cmd| to the sync thread and does not
    //   wait for the result, returning immediately after.
    GLint sendAndWaitForResult(SyncThreadCmd& cmd);
    void sendAsync(SyncThreadCmd& cmd);

    // |doSyncThreadCmd| and related functions below
    // execute the actual commands. These run on the sync thread.
    GLint doSyncThreadCmd(SyncThreadCmd* cmd);
    void doSyncContextInit();
    void doSyncWait(SyncThreadCmd* cmd);
    void doExit();

    // EGL objects / object handles specific to
    // a sync thread.
    EGLDisplay mDisplay;
    RenderThreadInfo* mTLS;
    uint32_t mContext;
    uint32_t mSurf;
};

