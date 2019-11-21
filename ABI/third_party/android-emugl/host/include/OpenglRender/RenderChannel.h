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
#pragma once

#include "android/base/containers/SmallVector.h"
#include "android/base/EnumFlags.h"
#include "android/base/files/Stream.h"

#include <functional>
#include <memory>

namespace emugl {

// Turn the RenderChannel::State enum into flags.
using namespace ::android::base::EnumFlags;

// RenderChannel - For each guest-to-host renderer connection, this provides
// an interface for the guest side to interact with the corresponding renderer
// thread on the host. Its main purpose is to send and receive wire protocol
// bytes in an asynchronous way (compatible with Android pipes).
//
// Usage is the following:
//    1) Get an instance pointer through a dedicated Renderer function
//       (e.g. RendererImpl::createRenderChannel()).
//
//    2) Call setEventCallback() to indicate which callback should be called
//       when the channel's state has changed due to a host thread event.
//
class RenderChannel {
public:
    // A type used to pass byte packets between the guest and the
    // RenderChannel instance. Experience has shown that using a
    // SmallFixedVector<char, N> instance instead of a std::vector<char>
    // avoids a lot of un-necessary heap allocations. The current size
    // of 512 was selected after profiling existing traffic, including
    // the one used in protocol-heavy benchmark like Antutu3D.
    using Buffer = android::base::SmallFixedVector<char, 512>;

    // Bit-flags for the channel state.
    // |CanRead| means there is data from the host to read.
    // |CanWrite| means there is room to send data to the host.
    // |Stopped| means the channel was stopped.
    enum class State {
        // Can't use None here, some system header declares it as a macro.
        Empty = 0,
        CanRead = 1 << 0,
        CanWrite = 1 << 1,
        Stopped = 1 << 2,
    };

    // Values corresponding to the result of i/o operations.
    // |Ok| means everything went well.
    // |TryAgain| means the operation could not be performed and should be
    // tried later.
    // |Error| means an error happened (i.e. the channel is stopped).
    enum class IoResult {
        Ok = 0,
        TryAgain = 1,
        Error = 2,
    };

    // Type of a callback used to tell the guest when the RenderChannel
    // state changes. Used by setEventCallback(). The parameter contains
    // the State bits matching the event, i.e. it is the logical AND of
    // the last value passed to setWantedEvents() and the current
    // RenderChannel state.
    using EventCallback = std::function<void(State)>;

    // Sets a single (!) callback that is called when the channel state's
    // changes due to an event *from* *the* *host* only. |callback| is a
    // guest-provided callback that will be called from the host renderer
    // thread, not the guest one.
    virtual void setEventCallback(EventCallback&& callback) = 0;

    // Used to indicate which i/o events the guest wants to be notified
    // through its StateChangeCallback. |state| must be a combination of
    // State::CanRead or State::CanWrite only. This will *not* call the
    // callback directly since this happens in the guest thread.
    virtual void setWantedEvents(State state) = 0;

    // Get the current state flags.
    virtual State state() const = 0;

    // Try to writes the data in |buffer| into the channel. On success,
    // return IoResult::Ok and moves |buffer|. On failure, return
    // IoResult::TryAgain if the channel was full, or IoResult::Error
    // if it is stopped.
    virtual IoResult tryWrite(Buffer&& buffer) = 0;

    // Try to read data from the channel. On success, return IoResult::Ok and
    // sets |*buffer| to contain the data. On failure, return
    // IoResult::TryAgain if the channel was empty, or IoResult::Error if
    // it was stopped.
    virtual IoResult tryRead(Buffer* buffer) = 0;

    // Abort all pending operations. Any following operation is a noop.
    // Once a channel is stopped, it cannot be re-started.
    virtual void stop() = 0;

    // Callback function when snapshotting the virtual machine.
    virtual void onSave(android::base::Stream* stream) = 0;

protected:
    ~RenderChannel() = default;
};

// Shared pointer to RenderChannel instance.
using RenderChannelPtr = std::shared_ptr<RenderChannel>;

}  // namespace emugl
