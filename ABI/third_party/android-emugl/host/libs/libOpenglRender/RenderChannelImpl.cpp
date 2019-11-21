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
#include "RenderChannelImpl.h"

#include "RenderThread.h"
#include "android/base/synchronization/Lock.h"

#include <algorithm>
#include <utility>

#include <assert.h>
#include <string.h>

#define EMUGL_DEBUG_LEVEL 0
#include "emugl/common/debug.h"

namespace emugl {

using Buffer = RenderChannel::Buffer;
using IoResult = RenderChannel::IoResult;
using State = RenderChannel::State;
using AutoLock = android::base::AutoLock;

// These constants correspond to the capacities of buffer queues
// used by each RenderChannelImpl instance. Benchmarking shows that
// it's important to have a large queue for guest -> host transfers,
// but a much smaller one works for host -> guest ones.
// Note: 32-bit Windows just doesn't have enough RAM to allocate optimal
// capacity.
#if defined(_WIN32) && !defined(_WIN64)
static constexpr size_t kGuestToHostQueueCapacity = 32U;
#else
static constexpr size_t kGuestToHostQueueCapacity = 1024U;
#endif
static constexpr size_t kHostToGuestQueueCapacity = 16U;

RenderChannelImpl::RenderChannelImpl(android::base::Stream* loadStream)
    : mFromGuest(kGuestToHostQueueCapacity, mLock),
      mToGuest(kHostToGuestQueueCapacity, mLock) {
    if (loadStream) {
        mFromGuest.onLoadLocked(loadStream);
        mToGuest.onLoadLocked(loadStream);
        mState = (State)loadStream->getBe32();
        mWantedEvents = (State)loadStream->getBe32();
#ifndef NDEBUG
        // Make sure we're in a consistent state after loading.
        const auto state = mState;
        updateStateLocked();
        assert(state == mState);
#endif
    } else {
        updateStateLocked();
    }
    // 创建线程
    mRenderThread.reset(new RenderThread(this, loadStream));
    mRenderThread->start();
}

void RenderChannelImpl::setEventCallback(EventCallback&& callback) {
    mEventCallback = std::move(callback);
    notifyStateChangeLocked();
}

void RenderChannelImpl::setWantedEvents(State state) {
    D("state=%d", (int)state);
    AutoLock lock(mLock);
    mWantedEvents |= state;
    notifyStateChangeLocked();
}

RenderChannel::State RenderChannelImpl::state() const {
    AutoLock lock(mLock);
    return mState;
}

IoResult RenderChannelImpl::tryWrite(Buffer&& buffer) {
    D("buffer size=%d", (int)buffer.size());
    AutoLock lock(mLock);
    auto result = mFromGuest.tryPushLocked(std::move(buffer));
    updateStateLocked();
    DD("mFromGuest.tryPushLocked() returned %d, state %d", (int)result,
       (int)mState);
    return result;
}

IoResult RenderChannelImpl::tryRead(Buffer* buffer) {
    D("enter");
    AutoLock lock(mLock);
    auto result = mToGuest.tryPopLocked(buffer);
    updateStateLocked();
    DD("mToGuest.tryPopLocked() returned %d, buffer size %d, state %d",
       (int)result, (int)buffer->size(), (int)mState);
    return result;
}

void RenderChannelImpl::stop() {
    D("enter");
    AutoLock lock(mLock);
    mFromGuest.closeLocked();
    mToGuest.closeLocked();
    mEventCallback = [](State state) {};
}

bool RenderChannelImpl::writeToGuest(Buffer&& buffer) {
    D("buffer size=%d", (int)buffer.size());
    AutoLock lock(mLock);
    IoResult result = mToGuest.pushLocked(std::move(buffer));
    updateStateLocked();
    D("mToGuest.pushLocked() returned %d, state %d", (int)result, (int)mState);
    notifyStateChangeLocked();
    return result == IoResult::Ok;
}

IoResult RenderChannelImpl::readFromGuest(Buffer* buffer, bool blocking) {
    D("enter");
    AutoLock lock(mLock);
    IoResult result;
    if (blocking) {
        result = mFromGuest.popLocked(buffer);
    } else {
        result = mFromGuest.tryPopLocked(buffer);
    }
    updateStateLocked();
    DD("mFromGuest.%s() return %d, buffer size %d, state %d",
       blocking ? "popLocked" : "tryPopLocked", (int)result,
       (int)buffer->size(), (int)mState);
    notifyStateChangeLocked();
    return result;
}

void RenderChannelImpl::stopFromHost() {
    D("enter");

    AutoLock lock(mLock);
    mFromGuest.closeLocked();
    mToGuest.closeLocked();
    mState |= State::Stopped;
    notifyStateChangeLocked();
    mEventCallback = [](State state) {};
}

bool RenderChannelImpl::isStopped() const {
    AutoLock lock(mLock);
    return (mState & State::Stopped) != 0;
}

RenderThread* RenderChannelImpl::renderThread() const {
    return mRenderThread.get();
}

void RenderChannelImpl::pausePreSnapshot() {
    AutoLock lock(mLock);
    mFromGuest.setSnapshotModeLocked(true);
    mToGuest.setSnapshotModeLocked(true);
}

void RenderChannelImpl::resume() {
    AutoLock lock(mLock);
    mFromGuest.setSnapshotModeLocked(false);
    mToGuest.setSnapshotModeLocked(false);
}

RenderChannelImpl::~RenderChannelImpl() {
    // Make sure the render thread is stopped before the channel is gone.
    mRenderThread->wait();
}

void RenderChannelImpl::updateStateLocked() {
    State state = RenderChannel::State::Empty;

    if (mToGuest.canPopLocked()) {
        state |= State::CanRead;
    }
    if (mFromGuest.canPushLocked()) {
        state |= State::CanWrite;
    }
    if (mToGuest.isClosedLocked()) {
        state |= State::Stopped;
    }
    mState = state;
}

void RenderChannelImpl::notifyStateChangeLocked() {
    // Always report stop events, event if not explicitly asked for.
    State available = mState & (mWantedEvents | State::Stopped);
    if (available != 0) {
        D("callback with %d", (int)available);
        mWantedEvents &= ~mState;
        mEventCallback(available);
    }
}

void RenderChannelImpl::onSave(android::base::Stream* stream) {
    D("enter");
    AutoLock lock(mLock);
    mFromGuest.onSaveLocked(stream);
    mToGuest.onSaveLocked(stream);
    stream->putBe32(static_cast<uint32_t>(mState));
    stream->putBe32(static_cast<uint32_t>(mWantedEvents));
    lock.unlock();
    mRenderThread->save(stream);
}

}  // namespace emugl
