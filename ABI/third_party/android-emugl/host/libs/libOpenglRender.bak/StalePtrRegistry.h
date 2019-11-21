/*
* Copyright (C) 2017 The Android Open Source Project
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

#include "android/base/containers/Lookup.h"
#include "android/base/files/Stream.h"
#include "android/base/files/StreamSerializing.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/Compiler.h"

#include <algorithm>
#include <cstdint>
#include <unordered_map>

// The purpose of StalePtrRegistry is to track integers corresponding to
// host-side pointers that may be invalidated after snapshots.
template <class T>
class StalePtrRegistry {
public:
    StalePtrRegistry() = default;

    void addPtr(T* ptr) {
        android::base::AutoWriteLock lock(mLock);
        mPtrs[asHandle(ptr)] = { ptr, Staleness::Live };
    }

    void removePtr(T* ptr) {
        android::base::AutoWriteLock lock(mLock);
        uint64_t handle = asHandle(ptr);
        mPtrs.erase(handle);
    }

    void remapStalePtr(uint64_t handle, T* newptr) {
        android::base::AutoWriteLock lock(mLock);
        mPtrs[handle] = { newptr, Staleness::PrevSnapshot };
    }

    T* getPtr(uint64_t handle, T* defaultPtr = nullptr,
              bool removeFromStaleOnGet = false) {
        android::base::AutoReadLock lock(mLock);

        // return |defaultPtr| if not found.
        T* res = defaultPtr;

        Entry* it = nullptr;

        if ((it = android::base::find(mPtrs, handle)))
            res = it->ptr;

        if (removeFromStaleOnGet &&
            it && it->staleness == Staleness::PrevSnapshot) {
            lock.unlockRead();
            android::base::AutoWriteLock wrlock(mLock);
            mPtrs.erase(handle);
        }

        return res;
    }

    void makeCurrentPtrsStale() {
        android::base::AutoWriteLock lock(mLock);
        for (auto& it : mPtrs) {
            it.second.staleness =
                Staleness::PrevSnapshot;
        }
    }

    size_t numCurrEntries() const {
        return countWithStaleness(Staleness::Live);
    }

    size_t numStaleEntries() const {
        return countWithStaleness(Staleness::PrevSnapshot);
    }

    void onSave(android::base::Stream* stream) {
        android::base::AutoReadLock lock(mLock);
        saveCollection(
                stream, mPtrs,
                [](android::base::Stream* stream,
                   const std::pair<uint64_t, Entry>& entry) {
                    stream->putBe64(entry.first);
                });
    }

    void onLoad(android::base::Stream* stream) {
        android::base::AutoWriteLock lock(mLock);
        loadCollection(
                stream, &mPtrs,
                [](android::base::Stream* stream) {
                    uint64_t handle = stream->getBe64();
                    return std::make_pair(
                               handle,
                               (Entry){ nullptr, Staleness::PrevSnapshot });
                });
    }
private:
    static uint64_t asHandle(const T* ptr) {
        return (uint64_t)(uintptr_t)ptr;
    }

    static T* asPtr(uint64_t handle) {
        return (T*)(uintptr_t)handle;
    }

    enum class Staleness {
        Live,
        PrevSnapshot,
    };
    struct Entry {
        T* ptr;
        Staleness staleness;
    };

    using PtrMap = std::unordered_map<uint64_t, Entry>;

    size_t countWithStaleness(Staleness check) const {
        android::base::AutoReadLock lock(mLock);
        return std::count_if(mPtrs.begin(), mPtrs.end(),
                   [check](const typename PtrMap::value_type& entry) {
                       return entry.second.staleness == check;
                   });
    }

    mutable android::base::ReadWriteLock mLock;
    PtrMap mPtrs;

    DISALLOW_COPY_AND_ASSIGN(StalePtrRegistry);
};
