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

#include "android/emulation/DmaMap.h"

#include "android/base/containers/Lookup.h"
#include "android/base/files/StreamSerializing.h"
#include "android/base/memory/LazyInstance.h"
#include "android/emulation/android_pipe_device.h"

#include <type_traits>
#include <inttypes.h>
#include <stdio.h>

#define DEBUG 0

#if DEBUG >= 1
#define D(fmt,...) fprintf(stderr, "DmaMap: %s: " fmt "\n", __func__, ##__VA_ARGS__);
#else
#define D(...) (void)0
#endif

#if DEBUG >= 2
#define DD(fmt,...) fprintf(stderr, "DmaMap: %s: " fmt "\n", __func__, ##__VA_ARGS__);
#else
#define DD(...) (void)0
#endif

#define E(fmt,...) fprintf(stderr, "DmaMap: ERROR: %s: " fmt "\n", __func__, ##__VA_ARGS__);

namespace android {

static DmaMap* sInstance = nullptr;

DmaMap* DmaMap::get() {
    return sInstance;
}

DmaMap* DmaMap::set(DmaMap* dmaMap) {
    DmaMap* result = sInstance;
    sInstance = dmaMap;
    return result;
}

void DmaMap::addBuffer(void* hwpipe,
                       uint64_t guest_paddr,
                       uint64_t bufferSize) {
    D("guest paddr 0x%llx bufferSize %llu",
      (unsigned long long)guest_paddr,
      (unsigned long long)bufferSize);
    DmaBufferInfo info;
    info.hwpipe = hwpipe;
    info.guestAddr = guest_paddr; // guest address
    info.bufferSize = bufferSize; // size of buffer
    info.currHostAddr = kNullopt; // no current host address
    android::base::AutoWriteLock lock(mLock);
    createMappingLocked(&info);
    mDmaBuffers[guest_paddr] = info;
}

void DmaMap::removeBuffer(uint64_t guest_paddr) {
    D("guest paddr 0x%llx", (unsigned long long)guest_paddr);
    android::base::AutoWriteLock lock(mLock);
    if (auto info = android::base::find(mDmaBuffers, guest_paddr)) {
        removeMappingLocked(info);
        mDmaBuffers.erase(guest_paddr);
    } else {
        E("guest addr 0x%llx not alloced!",
          (unsigned long long)guest_paddr);
    }
}

void* DmaMap::getHostAddr(uint64_t guest_paddr) {
    DD("guest paddr 0x%llx", (unsigned long long)guest_paddr);
    android::base::AutoReadLock rlock(mLock);
    if (auto info = android::base::find(mDmaBuffers, guest_paddr)) {
        if (info->currHostAddr) {
            DD("guest paddr 0x%llx -> host 0x%llx valid",
              (unsigned long long)guest_paddr,
              (unsigned long long)(*info->currHostAddr));
            return *(info->currHostAddr);
        } else {
            rlock.unlockRead();
            android::base::AutoWriteLock wlock(mLock);
            createMappingLocked(info);
            D("guest paddr 0x%llx -> host 0x%llx valid (new)",
              (unsigned long long)guest_paddr,
              (unsigned long long)*(info->currHostAddr));
            return *(info->currHostAddr);
        }
    } else {
        E("guest paddr 0x%llx not alloced!",
          (unsigned long long)guest_paddr);
        return 0;
    }
}

void DmaMap::invalidateHostMappings() {
    android::base::AutoWriteLock lock(mLock);
    for (auto& it : mDmaBuffers) {
        removeMappingLocked(&it.second);
    }
}

void DmaMap::resetHostMappings() {
    android::base::AutoWriteLock lock(mLock);
    for (auto& it : mDmaBuffers) {
        removeMappingLocked(&it.second);
    }
    mDmaBuffers.clear();
}

void* DmaMap::getPipeInstance(uint64_t guest_paddr) {
    android::base::AutoReadLock lock(mLock);
    if (auto info = android::base::find(mDmaBuffers, guest_paddr)) {
        return info->hwpipe;
    } else {
        E("guest paddr 0x%llx not alloced!",
          (unsigned long long)guest_paddr);
        return nullptr;
    }
}

void DmaMap::createMappingLocked(DmaBufferInfo* info) {
    info->currHostAddr = doMap(info->guestAddr, info->bufferSize);
}

void DmaMap::removeMappingLocked(DmaBufferInfo* info ) {
    if (info->currHostAddr) {
        doUnmap(*(info->currHostAddr), info->bufferSize);
        info->currHostAddr = kNullopt;
        D("guest addr 0x%llx host mapping 0x%llx removed.",
          (unsigned long long)info->guestAddr,
          (unsigned long long)info->currHostAddr);
    } else {
        D("guest addr 0x%llx has no host mapping. don't remove.",
          (unsigned long long)info->guestAddr);
    }
}

void DmaMap::save(android::base::Stream* stream) const {
    saveCollection(stream, mDmaBuffers,
                   [](android::base::Stream* stream,
                      const DmaBufferMap::value_type& v) {
        stream->putBe64(v.first); // guest paddr
        stream->putBe32(android_pipe_get_id(v.second.hwpipe));
        stream->putBe64(v.second.guestAddr); // guest addr
        stream->putBe64(v.second.bufferSize); // buffer size
        // don't save current host addr as it is invalidated.
    });
}

void DmaMap::load(android::base::Stream* stream) {
    mDmaBuffers.clear();
    loadCollection(stream, &mDmaBuffers,
                   [](android::base::Stream* stream) {
        uint64_t gpa = stream->getBe64();

        DmaBufferInfo info;
        info.hwpipe = android_pipe_lookup_by_id(stream->getBe32()),
        info.guestAddr = stream->getBe64(),
        info.bufferSize = stream->getBe64(),
        info.currHostAddr = kNullopt;
        return std::make_pair(gpa, info);
    });
}

}  // namespace android


