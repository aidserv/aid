// Copyright (C) 2014 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN 1
#  include <windows.h>
#else
#  include <pthread.h>
#endif

#include <stdint.h>

namespace android {
namespace base {

// Very basic platform thread wrapper that only uses a tiny stack.
// This shall only be used during unit testing, and allows test code
// to not depend on the implementation of android::base::Thread.
class TestThread {
public:
    // Main thread function type.
    typedef void* (ThreadFunction)(void* param);

    // Constructor actually launches a new platform thread.
    TestThread(ThreadFunction* func, void* funcParam,
               int stackSize = 16384) {
#ifdef _WIN32
        mThread = CreateThread(NULL,
                               stackSize,
                               (DWORD WINAPI (*)(void*))func,
                               funcParam,
                               0,
                               NULL);
#else
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setstacksize(&attr, stackSize);
        pthread_create(&mThread,  &attr, func, funcParam);
        pthread_attr_destroy(&attr);
#endif
    }

    ~TestThread() {
#ifdef _WIN32
        CloseHandle(mThread);
#endif
    }

    intptr_t join() {
#ifdef _WIN32
        WaitForSingleObject(mThread, INFINITE);
        DWORD ret = 0;
        ::GetExitCodeThread(mThread, &ret);
#else
        void* ret = NULL;
        pthread_join(mThread, &ret);
#endif
        return (intptr_t)ret;
    }

private:
#ifdef _WIN32
    HANDLE mThread;
#else
    pthread_t mThread;
#endif
};

}  // namespace base
}  // namespace android
