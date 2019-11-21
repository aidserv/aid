// Copyright (C) 2016 The Android Open Source Project
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

#include "android/base/synchronization/Lock.h"

#include "android/base/testing/TestThread.h"
#include "android/base/testing/Utils.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <random>
#include <utility>
#include <vector>

namespace android {
namespace base {

// On windows versions prior to Vista,
// this should use the fallback implementation.
// It should work for both 32 and 64 bit versions of the unit test
// (WINAPI (which changes calling convention)
// was needed to make the 32 bit version work)
TEST(ReadWriteLock, init) {
    ReadWriteLock rwl;
}

// basic lock/unlock for readers
TEST(ReadWriteLock, readLockBasic) {
    ReadWriteLock rwl;
    rwl.lockRead();
    rwl.unlockRead();
}

// basic lock/unlock for writer
TEST(ReadWriteLock, writeLockBasic) {
    ReadWriteLock rwl;
    rwl.lockWrite();
    rwl.unlockWrite();
}

// check we can lock reader multiple times from the same thread
TEST(ReadWriteLock, readLockMultiReadSingleThread) {
    ReadWriteLock rwl;
    rwl.lockRead();
    rwl.lockRead();
    rwl.lockRead();
    rwl.lockRead();
    rwl.unlockRead();
    rwl.unlockRead();
    rwl.unlockRead();
    rwl.unlockRead();
}

// Write lock tests.

// RWThreadParams class + thread fn inspired by Lock_unittest
// Package up data being modified + lock class

struct RWThreadParams {
    RWThreadParams() : mutex(), counter(0), read_counter(0) {}
    ReadWriteLock mutex;
    int counter;
    int read_counter;
};

static void* writeThreadFunction(void* param) {
    RWThreadParams* p = static_cast<RWThreadParams*>(param);

    p->mutex.lockWrite();
    p->counter++;
    p->mutex.unlockWrite();
    return NULL;
}

static void* readThreadFunction(void* param) {
    RWThreadParams* p = static_cast<RWThreadParams*>(param);

    p->mutex.lockRead();
    p->read_counter++;
    p->mutex.unlockRead();
    return NULL;
}

// basic synchronized write / read tests

// write: just like for Lock
TEST(ReadWriteLock, SyncWrite) {
    ReadWriteLock rwl;

    const size_t kNumWriteThreads = 200;
    std::vector<TestThread> write_threads;
    write_threads.reserve(kNumWriteThreads);
    RWThreadParams p;

    // Create and launch all threads.
    for (size_t i = 0; i < kNumWriteThreads; i++) {
        write_threads.emplace_back(writeThreadFunction, &p);
    }

    // Wait until their completion.
    for (size_t i = 0; i < kNumWriteThreads; ++i) {
        write_threads[i].join();
    }

    EXPECT_EQ(static_cast<int>(kNumWriteThreads), p.counter);
}

// Have half of the threads read/write.
// Launch them in various ways (interleaved,
// all writes first, all reads first, random)

TEST(ReadWriteLock, SyncReadWrite) {
    const size_t kNumThreads = 100;
    std::vector<TestThread> write_threads;
    std::vector<TestThread> read_threads;
    write_threads.reserve(kNumThreads);
    read_threads.reserve(kNumThreads);

    RWThreadParams p;

    // Create and launch all threads.
    for (size_t i = 0; i < kNumThreads; ++i) {
        write_threads.emplace_back(writeThreadFunction, &p);
        read_threads.emplace_back(readThreadFunction, &p);
    }

    // Wait until their completion.
    for (size_t n = 0; n < kNumThreads; ++n) {
        write_threads[n].join();
        read_threads[n].join();
    }

    EXPECT_EQ(static_cast<int>(kNumThreads), p.counter);
    EXPECT_GE(static_cast<int>(kNumThreads), p.read_counter);

    p.counter = 0;
    p.read_counter = 0;

    // Create and launch all threads.
    write_threads.clear();
    read_threads.clear();
    for (size_t i = 0; i < kNumThreads; ++i) {
        write_threads.emplace_back(writeThreadFunction, &p);
    }
    for (size_t i = 0; i < kNumThreads; ++i) {
        read_threads.emplace_back(readThreadFunction, &p);
    }

    // Wait until their completion.
    for (size_t i = 0; i < kNumThreads; ++i) {
        write_threads[i].join();
        read_threads[i].join();
    }

    EXPECT_EQ(static_cast<int>(kNumThreads), p.counter);
    EXPECT_GE(static_cast<int>(kNumThreads), p.read_counter);

    p.counter = 0;
    p.read_counter = 0;

    // Create and launch all threads.
    write_threads.clear();
    read_threads.clear();
    for (size_t i = 0; i < kNumThreads; ++i) {
        read_threads.emplace_back(readThreadFunction, &p);
    }
    for (size_t i = 0; i < kNumThreads; ++i) {
        write_threads.emplace_back(writeThreadFunction, &p);
    }

    // Wait until their completion.
    for (size_t i = 0; i < kNumThreads; ++i) {
        write_threads[i].join();
        read_threads[i].join();
    }

    EXPECT_EQ(static_cast<int>(kNumThreads), p.counter);
    EXPECT_GE(static_cast<int>(kNumThreads), p.read_counter);
}

TEST(ReadWriteLock, SyncReadWriteRandom) {
    const size_t kTrials = 10;
    const size_t kNumThreadsPerTrial = 100;
    RWThreadParams p;

    std::default_random_engine generator;

    // At least one writer and reader
    std::uniform_int_distribution<int> writers(1, kNumThreadsPerTrial - 1);
    std::vector<TestThread*> threads(kNumThreadsPerTrial);

    for (size_t i = 0; i < kTrials; i++) {
        p.counter = 0;
        p.read_counter = 0;

        size_t num_writers = writers(generator);
        size_t num_readers = kNumThreadsPerTrial - num_writers;

        std::vector<std::pair<int, bool> > indices_writers;
        for (size_t i = 0; i < kNumThreadsPerTrial; i++) {
            std::pair<int, bool> item;
            item.first = i;
            item.second = i < num_writers;
            indices_writers.push_back(item);
        }
        std::random_shuffle(indices_writers.begin(), indices_writers.end());

        for (size_t i = 0; i < kNumThreadsPerTrial; i++) {
            if (indices_writers[i].second) {
                threads[indices_writers[i].first] =
                    new TestThread(writeThreadFunction, &p);
            } else {
                threads[indices_writers[i].first] =
                    new TestThread(readThreadFunction, &p);
            }
        }

        for (size_t i = 0; i < kNumThreadsPerTrial; i++) {
            threads[i]->join();
            delete threads[i];
        }

        EXPECT_EQ(num_writers, (size_t)p.counter);
        // Simultaneous reader threads can increment old versions
        // of read_counter, so this is allowed to be >=.
        EXPECT_GE(num_readers, (size_t)p.read_counter);
    }
}

}  // namespace base
}  // namespace android
