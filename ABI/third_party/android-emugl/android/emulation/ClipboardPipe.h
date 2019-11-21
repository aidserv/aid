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

#include "android/base/synchronization/Lock.h"
#include "android/emulation/AndroidPipe.h"

#include <functional>
#include <memory>
#include <vector>

namespace android {
namespace emulation {

// This is a special pipe for updating the contents of guest system's
// clipboard. The other side is connected to Android's clipboard service
// that has been patched to listen messages from it.
class ClipboardPipe final : public AndroidPipe {
public:
    using GuestClipboardCallback = std::function<void(const uint8_t*, size_t)>;
    using Ptr = std::shared_ptr<ClipboardPipe>;

    class Service final : public AndroidPipe::Service {
    public:
        Service();
        AndroidPipe* create(void* hwPipe, const char* args) override;

        static ClipboardPipe::Ptr getPipe();
        static void closePipe();
    };

    ClipboardPipe(void* hwPipe, Service* svc);

    void onGuestClose(PipeCloseReason reason) override;
    unsigned onGuestPoll() const override;
    int onGuestRecv(AndroidPipeBuffer* buffers, int numBuffers) override;
    int onGuestSend(const AndroidPipeBuffer* buffers, int numBuffers) override;
    void onGuestWantWakeOn(int flags) override {
        mWakeOnRead = (flags & PIPE_WAKE_READ) != 0;
        wakeGuestIfNeeded();
    }

    static void setEnabled(bool enabled);
    static void setGuestClipboardCallback(GuestClipboardCallback cb);
    void setGuestClipboardContents(const uint8_t* buf, size_t len);

private:
    // A state of read or write operation, encapsulating the data size + buffer
    // + the transfer position.
    struct ReadWriteState {
        std::vector<uint8_t> buffer;
        uint32_t dataSize;
        uint32_t processedBytes;
        bool dataSizeTransferred;

        ReadWriteState();

        // Return the data buffer to transfer and its size.
        uint8_t* data();
        int size() const;

        // Check if the state's buffer is finished (nothing left to transfer)
        bool isFinished() const;

        // Reset the state so it's safe to start a new transfer.
        void reset();
    };

    // A class that manages the contents writing to the guest + potential update
    // of clipboard in the middle of the transfer because of asynchronous host
    // event.
    //
    // It holds two ReadWriteState objects: the one that's being written right
    // now - |inProgress|, - and another one, queued to be written later -
    // |queued|.
    //
    // When new data arrives, it goes into |queued| state; new data coming would
    // keep going there, overwriting the previous one, until the guest initiates
    // a transfer. At that point |queued| moves into |inProgress| and that's
    // what get written to guest. |inProgress| never changes until it's
    // completely written out.
    //
    // This design makes sure guest can't get confused when it reads the first
    // 4 bytes - data size - and then user updates clipboard contents to
    // something much smaller, so emulator can't fill the whole buffer guest
    // expected to get filled. Instead, we make sure to complete the queued
    // operation first, and only then tell the guest to read again as we've
    // got more data for it.
    //
    struct WritingState {
        WritingState();

        void queueContents(const uint8_t* buf, size_t len);
        ReadWriteState* pickStateWithData();
        void clearQueued();
        bool hasData() const;

    private:
        ReadWriteState* inProgress;
        ReadWriteState* queued;
        ReadWriteState states[2];  // the actual states storage
        mutable android::base::Lock lock;
    };

    void wakeGuestIfNeeded();

    enum class OperationType { ReadFromGuest, WriteToGuest };
    int processOperation(OperationType operation,
                         ReadWriteState* state,
                         const AndroidPipeBuffer* pipeBuffers,
                         int numPipeBuffers);

    WritingState mGuestWriteState;
    ReadWriteState mGuestReadState;
    bool mWakeOnRead = false;

    static bool sEnabled;
};

void registerClipboardPipeService();

}  // namespace emulation
}  // namespace android
