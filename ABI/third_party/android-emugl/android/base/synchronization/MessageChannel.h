// Copyright 2014 The Android Open Source Project
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

#include "android/base/Optional.h"
#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/synchronization/Lock.h"

#include <utility>
#include <stddef.h>

namespace android {
namespace base {

// Base non-templated class used to reduce the amount of template
// specialization.
class MessageChannelBase {
public:
    // Get the current channel size
    size_t size() const;

    // Abort the currently pending operations and don't allow any other ones
    void stop();

    // Check if the channel is stopped.
    bool isStopped() const;

    // Block until the channel has no pending messages.
    void waitForEmpty();

protected:
    // Constructor. |capacity| is the buffer capacity in messages.
    MessageChannelBase(size_t capacity);

    // Destructor.
    ~MessageChannelBase() = default;

    // Call this method in the sender thread before writing a new message.
    // This returns the position of the available slot in the message array
    // where to copy the new fixed-size message. After the copy, call
    // afterWrite().
    // If the channel is stopped, return value is undefined.
    size_t beforeWrite();

    // Same as beforeWrite(), but returns an empty optional if there was
    // no room to write to instead of waiting for it.
    // One still needs to call afterWrite() anyway.
    Optional<size_t> beforeTryWrite();

    // To be called after trying to write a new fixed-size message (which should
    // happen after beforeWrite() or beforeTryWrite()).
    // |success| must be true to indicate that a new item was added to the
    // channel, or false otherwise (i.e. if the channel is stopped, or if
    // beforeTryWrite() returned an empty optional).
    void afterWrite(bool success);

    // Call this method in the receiver thread before reading a new message.
    // This returns the position in the message array where the new message
    // can be read. Caller must process the message, then call afterRead().
    // If the channel is stopped, return value is undefined.
    size_t beforeRead();

    // Same as beforeRead(), but returns an empty optional if there was
    // no data to read instead of waiting for it.
    // One still needs to call afterWrite() anyway.
    Optional<size_t> beforeTryRead();

    // Same as beforeRead(), but returns an empty optional if no data arrived
    // by the |wallTimeUs| absolute time. One still needs to call
    // afterWrite() anyway.
    Optional<size_t> beforeTimedRead(System::Duration wallTimeUs);

    // To be called after reading a fixed-size message from the channel (which
    // must happen after beforeRead() or beforeTryRead()).
    // |success| must be true to indicate that a message was read, or false
    // otherwise (i.e. if the channel is stopped or if beforeTryRead() returned
    // an empty optional).
    void afterRead(bool success);

    // A version of isStopped() that doesn't lock the channel but expects it
    // to be locked by the caller.
    bool isStoppedLocked() const { return mStopped; }

private:
    size_t mPos = 0;
    size_t mCapacity;
    size_t mCount = 0;
    bool mStopped = false;
    mutable Lock mLock;     // Mutable to allow const members to lock it.
    ConditionVariable mCanRead;
    ConditionVariable mCanWrite;
};

// Helper class used to implement an uni-directional IPC channel between
// two threads. The channel can be used to send fixed-size messages of type
// |T|, with an internal buffer size of |CAPACITY| items. All calls are
// blocking.
//
// Usage is pretty straightforward:
//
//   - From the sender thread, call send(msg);
//   - From the receiver thread, call receive(&msg);
//   - If you want to stop the IPC, call stop();
template <typename T, size_t CAPACITY>
class MessageChannel : public MessageChannelBase {
public:
    MessageChannel() : MessageChannelBase(CAPACITY) {}

    bool send(const T& msg) {
        const size_t pos = beforeWrite();
        const bool res = !isStoppedLocked();
        if (res) {
            mItems[pos] = msg;
        }
        afterWrite(res);
        return res;
    }

    bool send(T&& msg) {
        const size_t pos = beforeWrite();
        const bool res = !isStoppedLocked();
        if (res) {
            mItems[pos] = std::move(msg);
        }
        afterWrite(res);
        return res;
    }

    bool trySend(const T& msg) {
        const auto pos = beforeTryWrite();
        if (pos) {
            mItems[*pos] = msg;
        }
        afterWrite(pos);
        return pos;
    }

    bool trySend(T&& msg) {
        const auto pos = beforeTryWrite();
        if (pos) {
            mItems[*pos] = std::move(msg);
        }
        afterWrite(pos);
        return pos;
    }

    bool receive(T* msg) {
        const size_t pos = beforeRead();
        const bool res = !isStoppedLocked();
        if (res) {
            *msg = std::move(mItems[pos]);
        }
        afterRead(res);
        return res;
    }

    Optional<T> receive() {
        const size_t pos = beforeRead();
        if (!isStoppedLocked()) {
            Optional<T> msg(std::move(mItems[pos]));
            afterRead(true);
            return msg;
        } else {
            afterRead(false);
            return {};
        }
    }

    bool tryReceive(T* msg) {
        const auto pos = beforeTryRead();
        if (pos) {
            *msg = std::move(mItems[*pos]);
        }
        afterRead(pos);
        return pos;
    }

    Optional<T> timedReceive(System::Duration wallTimeUs) {
        const auto pos = beforeTimedRead(wallTimeUs);
        if (pos && !isStoppedLocked()) {
            Optional<T> res(std::move(mItems[*pos]));
            afterRead(true);
            return res;
        }
        afterRead(false);
        return {};
    }

    constexpr size_t capacity() const { return CAPACITY; }

private:
    T mItems[CAPACITY];
};

}  // namespace base
}  // namespace android
