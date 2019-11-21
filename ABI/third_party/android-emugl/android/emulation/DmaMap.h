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

#pragma once

#include "android/base/Compiler.h"
#include "android/base/files/Stream.h"
#include "android/base/Optional.h"
#include "android/base/synchronization/Lock.h"

#include <unordered_map>
#include <inttypes.h>

using android::base::Optional;
using android::base::kNullopt;

namespace android {

struct DmaBufferInfo {
    void* hwpipe = nullptr;
    uint64_t guestAddr = 0;
    uint64_t bufferSize = 0;
    Optional<void*> currHostAddr = kNullopt;
};

using DmaBufferMap = std::unordered_map<uint64_t, DmaBufferInfo>;

class DmaMap {
public:
    // Constructor.
    DmaMap() = default;

    // Destructor.
    virtual ~DmaMap() = default;

    void addBuffer(void* hwpipe, uint64_t addr, uint64_t bufferSize);
    void removeBuffer(uint64_t addr);
    void* getHostAddr(uint64_t addr);
    void invalidateHostMappings();
    void resetHostMappings();
    void* getPipeInstance(uint64_t addr);

    // get() / set are NOT thread-safe,
    // but we don't expect multiple threads to call this
    // concurrently at init time, and the worst that can happen is to leak
    // a single instance. (c.f. VmLock)
    static DmaMap* get();
    static DmaMap* set(DmaMap* dmaMap);

    // Snapshotting.
    void save(android::base::Stream* stream) const;
    void load(android::base::Stream* stream);
protected:

    virtual void createMappingLocked(DmaBufferInfo* info);
    virtual void removeMappingLocked(DmaBufferInfo* info);

    // These are meant to be replaced by a real implementation
    // in subclasses.
    virtual void* doMap(uint64_t addr, uint64_t bufferSize) = 0;
    virtual void doUnmap(void* mapped, uint64_t bufferSize) = 0;

    DmaBufferMap mDmaBuffers;
    android::base::ReadWriteLock mLock;
    DISALLOW_COPY_ASSIGN_AND_MOVE(DmaMap);
};

} // namespace android
