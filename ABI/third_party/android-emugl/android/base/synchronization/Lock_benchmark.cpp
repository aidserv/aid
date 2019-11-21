// Copyright 2016 The Android Open Source Project
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

// A small benchmark used to compare the performance of android::base::Lock
// with other mutex implementions.

#include "android/base/synchronization/Lock.h"

#include <mutex>
#include <atomic>

#include "benchmark/benchmark_api.h"

#define BASIC_BENCHMARK_TEST(x) \
    BENCHMARK(x)->Arg(8)->Arg(512)->Arg(8192)

void BM_BaseLock_LockUnlock(benchmark::State& state) {
    android::base::Lock lock;
    while (state.KeepRunning()) {
        lock.lock();
        lock.unlock();
    }
}

BENCHMARK(BM_BaseLock_LockUnlock);
BENCHMARK(BM_BaseLock_LockUnlock)->ThreadPerCpu();

struct BaseLockContention {
    int counter = 0;
    android::base::Lock lock;
};

void BM_StdMutex_LockUnlock(benchmark::State& state) {
    std::mutex lock;
    while (state.KeepRunning()) {
        lock.lock();
        lock.unlock();
    }
}

BENCHMARK(BM_StdMutex_LockUnlock);
BENCHMARK(BM_StdMutex_LockUnlock)->ThreadPerCpu();

static BaseLockContention sBaseContention;

void BM_BaseLock_Contention(benchmark::State& state) {
    static BaseLockContention contention;
    if (state.thread_index == 0) {
        contention.counter = 0;
    }
    while (state.KeepRunning()) {
        for (int i = 0; i < state.range_x(); ++i) {
            contention.lock.lock();
            contention.counter++;
            contention.lock.unlock();
        }
    }
}

BENCHMARK(BM_BaseLock_Contention)->Arg(32);
BENCHMARK(BM_BaseLock_Contention)->Arg(32)->ThreadPerCpu();

struct StdMutexContention {
    int counter = 0;
    std::mutex lock;
};

void BM_StdMutex_Contention(benchmark::State& state) {
    static StdMutexContention contention;
    if (state.thread_index == 0) {
        contention.counter = 0;
    }
    while (state.KeepRunning()) {
        for (int i = 0; i < state.range_x(); ++i) {
            contention.lock.lock();
            contention.counter++;
            contention.lock.unlock();
        }
    }
}

BENCHMARK(BM_StdMutex_Contention)->Arg(32);
BENCHMARK(BM_StdMutex_Contention)->Arg(32)->ThreadPerCpu();

static StdMutexContention sStdContention;


BENCHMARK_MAIN()
