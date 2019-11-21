// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include <vector>
#include <utility>
#include <type_traits>
#include <cassert>

namespace android {
namespace base {

// A CircularBuffer<T> is a wrapper over std::vector<T> of fixed size
// that has the following properties:
// 1. Only allows adding new elements via |push_back|
// 2. Items can be removed either from the front or from the back.
// 3. When at maximum capacity, the oldest element is removed to make
//    space for the new one.
template <class T, class A = std::allocator<T>>
class CircularBuffer {
public:
    using value_type = T;

    // Create a new circular buffer with the given |capacity|.
    explicit CircularBuffer(int capacity) :
        mBuf(capacity), mSize(0), mFrontIdx(0), mBackIdx(0) {
        assert(capacity > 0);
    }

    // Adds a new element at the end of the container. If the container
    // is at maximum capacity, the oldest element will be removed to make
    // space for the new one.
    template <class U>
    typename std::enable_if<std::is_convertible<U, T>::value>::type
    push_back(U&& value) {
        mBuf[mBackIdx] = std::forward<U>(value);
        incrementBackIdx();
    }

    // Returns the first element in the container.
    // Undefined behavior if called on an empty container.
    T& front() {
        assert(!empty());
        return mBuf[mFrontIdx];
    }

    // Same as above, const version.
    const T& front() const {
        assert(!empty());
        return mBuf[mFrontIdx];
    }

    // Returns the last element in the container.
    // Undefined behavior if called on an empty container.
    T& back() {
        assert(!empty());
        return mBuf[(mFrontIdx + mSize - 1) % mBuf.size()];
    }

    // Same as above, const version.
    const T& back() const {
        assert(!empty());
        return mBuf[(mFrontIdx + mSize - 1) % mBuf.size()];
    }

    // Removes the first element in the container.
    // Undefined behavior if called on an empty container.
    void pop_front() {
        assert(!empty());
        mSize--;
        mFrontIdx = (mFrontIdx + 1) % mBuf.size();
    }

    // Removes the last element in the container.
    // Undefined behavior if called on an empty container.
    void pop_back() {
        assert(!empty());
        mSize--;
        mBackIdx -= 1;
        if (mBackIdx < 0) mBackIdx = mBuf.size() - 1;
    }

    // Returns true if the container is empty, false otherwise.
    bool empty() const {
        return mSize == 0;
    }

    // Returns the number of elements in the buffer
    int size() const {
        return mSize;
    }

    // Get a specific element from the buffer.
    // |idx| must be between 0 and |size|.
    T& operator[](int idx) {
        return mBuf[(mFrontIdx + idx) % mBuf.size()];
    }

    // Same as above, const version.
    const T& operator[](int idx) const {
        return mBuf[(mFrontIdx + idx) % mBuf.size()];
    }

private:
    void incrementBackIdx() {
        if (mSize < static_cast<int>(mBuf.size())) {
            mSize++;
        } else {
           // Buffer is at full capacity, erase first
           // element.
           mFrontIdx = (mFrontIdx + 1) % mBuf.size();
        }
        mBackIdx = (mBackIdx + 1) % mBuf.size();
    }

    // Buffer containing the data.
    std::vector<T, A> mBuf;

    // Number of elements in the circular buffer.
    int mSize;

    // Index, at which the first (oldest) element of
    // the circular buffer is stored in mBuf.
    int mFrontIdx;

    // Index at which the next element will be placed
    // in mBuf.
    int mBackIdx;
};

}
}
