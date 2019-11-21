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
#include "ChannelStream.h"

#include "OpenglRender/RenderChannel.h"

#define EMUGL_DEBUG_LEVEL  0
#include "emugl/common/debug.h"
#include "emugl/common/dma_device.h"

#include <assert.h>
#include <memory.h>

namespace emugl {

using IoResult = RenderChannel::IoResult;

ChannelStream::ChannelStream(RenderChannelImpl* channel, size_t bufSize)
    : IOStream(bufSize), mChannel(channel) {
    mWriteBuffer.resize_noinit(bufSize);
}

void* ChannelStream::allocBuffer(size_t minSize) {
    if (mWriteBuffer.size() < minSize) {
        mWriteBuffer.resize_noinit(minSize);
    }
    return mWriteBuffer.data();
}

int ChannelStream::commitBuffer(size_t size) {
    assert(size <= mWriteBuffer.size());
    if (mWriteBuffer.isAllocated()) {
        mWriteBuffer.resize(size);
        mChannel->writeToGuest(std::move(mWriteBuffer));
    } else {
        mChannel->writeToGuest(
                RenderChannel::Buffer(mWriteBuffer.data(), mWriteBuffer.data() + size));
    }
    return size;
}

const unsigned char* ChannelStream::readRaw(void* buf, size_t* inout_len) {
    size_t wanted = *inout_len;
    size_t count = 0U;
    auto dst = static_cast<uint8_t*>(buf);
    D("wanted %d bytes", (int)wanted);
    while (count < wanted) {
        if (mReadBufferLeft > 0) {
            size_t avail = std::min<size_t>(wanted - count, mReadBufferLeft);
            memcpy(dst + count,
                   mReadBuffer.data() + (mReadBuffer.size() - mReadBufferLeft),
                   avail);
            count += avail;
            mReadBufferLeft -= avail;
            continue;
        }
        bool blocking = (count == 0);
        auto result = mChannel->readFromGuest(&mReadBuffer, blocking);
        D("readFromGuest() returned %d, size %d", (int)result, (int)mReadBuffer.size());
        if (result == IoResult::Ok) {
            mReadBufferLeft = mReadBuffer.size();
            continue;
        }
        if (count > 0) {  // There is some data to return.
            break;
        }
        // Result can only be IoResult::Error if |count| == 0
        // since |blocking| was true, it cannot be IoResult::TryAgain.
        assert(result == IoResult::Error);
        D("error while trying to read");
        return nullptr;
    }
    *inout_len = count;
    D("read %d bytes", (int)count);
    return (const unsigned char*)buf;
}

void* ChannelStream::getDmaForReading(uint64_t guest_paddr) {
    return g_emugl_dma_get_host_addr(guest_paddr);
}

void ChannelStream::unlockDma(uint64_t guest_paddr) {
    g_emugl_dma_unlock(guest_paddr);
}

void ChannelStream::forceStop() {
    mChannel->stopFromHost();
}

void ChannelStream::onSave(android::base::Stream* stream) {
    // Write only the data that's left in read buffer, but in the same format
    // as saveBuffer() does.
    stream->putBe32(mReadBufferLeft);
    stream->write(mReadBuffer.data() + mReadBuffer.size() - mReadBufferLeft,
                  mReadBufferLeft);
    android::base::saveBuffer(stream, mWriteBuffer);
}

unsigned char* ChannelStream::onLoad(android::base::Stream* stream) {
    android::base::loadBuffer(stream, &mReadBuffer);
    mReadBufferLeft = mReadBuffer.size();
    android::base::loadBuffer(stream, &mWriteBuffer);
    return reinterpret_cast<unsigned char*>(mWriteBuffer.data());
}

}  // namespace emugl
