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

#include "renderControl_enc.h"

#include <cutils/log.h>
#include <pthread.h>
#if PLATFORM_SDK_VERSION > 24
#include <system/qemu_pipe.h>
#else // PLATFORM_SDK_VERSION
#include <hardware/qemu_pipe.h>
#endif //PLATFORM_SDK_VERSION

static int                sProcPipe = 0;
static pthread_once_t     sProcPipeOnce = PTHREAD_ONCE_INIT;
// sProcUID is a unique ID per process assigned by the host.
// It is different from getpid().
static uint64_t           sProcUID = 0;

// processPipeInitOnce is used to generate a process unique ID (puid).
// processPipeInitOnce will only be called at most once per process.
// Use it with pthread_once for thread safety.
// The host associates resources with process unique ID (puid) for memory cleanup.
// It will fallback to the default path if the host does not support it.
// Processes are identified by acquiring a per-process 64bit unique ID from the
// host.
static void processPipeInitOnce() {
    sProcPipe = qemu_pipe_open("GLProcessPipe");
    if (sProcPipe < 0) {
        sProcPipe = 0;
        ALOGW("Process pipe failed");
        return;
    }
    // Send a confirmation int to the host
    int32_t confirmInt = 100;
    ssize_t stat = 0;
    do {
        stat = ::write(sProcPipe, (const char*)&confirmInt,
                sizeof(confirmInt));
    } while (stat < 0 && errno == EINTR);

    if (stat != sizeof(confirmInt)) { // failed
        close(sProcPipe);
        sProcPipe = 0;
        ALOGW("Process pipe failed");
        return;
    }

    // Ask the host for per-process unique ID
    do {
        stat = ::read(sProcPipe, (char*)&sProcUID,
                      sizeof(sProcUID));
    } while (stat < 0 && errno == EINTR);

    if (stat != sizeof(sProcUID)) {
        close(sProcPipe);
        sProcPipe = 0;
        sProcUID = 0;
        ALOGW("Process pipe failed");
        return;
    }
}

bool processPipeInit(renderControl_encoder_context_t *rcEnc) {
    pthread_once(&sProcPipeOnce, processPipeInitOnce);
    if (!sProcPipe) return false;
    rcEnc->rcSetPuid(rcEnc, sProcUID);
    return true;
}