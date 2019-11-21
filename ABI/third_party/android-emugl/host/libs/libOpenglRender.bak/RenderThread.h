/*
* Copyright (C) 2011 The Android Open Source Project
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

#include "android/base/files/MemStream.h"
#include "android/base/Optional.h"
#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/synchronization/Lock.h"
#include "emugl/common/mutex.h"
#include "emugl/common/thread.h"
#include "OpenglRender/IOStream.h"
#include <memory>
#include "TcpStream.h"

class IOStream;
namespace emugl {

class RenderChannelImpl;
class RendererImpl;
class ReadBuffer;

// A class used to model a thread of the RenderServer. Each one of them
// handles a single guest client / protocol byte stream.
class RenderThread : public emugl::Thread {
    using MemStream = android::base::MemStream;

public:
    // Create a new RenderThread instance.
    explicit RenderThread(RenderChannelImpl* channel,
                          android::base::Stream* loadStream = nullptr);
	// add
	explicit RenderThread(IOStream *myIOStream);
	
    virtual ~RenderThread();

    // Returns true iff the thread has finished.
    bool isFinished() const { return mFinished; }

    void pausePreSnapshot();
    void resume();
    void save(android::base::Stream* stream);
	// add
	IOStream *m_IOstream;
	//SocketStream *m_IOstream;
	TcpStream *m_listenSock;
private:
    virtual intptr_t main();
    void setFinished();

    // Snapshot support.
    enum class SnapshotState {
        Empty,
        StartSaving,
        StartLoading,
        InProgress,
        Finished,
    };

    template <class OpImpl>
    void snapshotOperation(android::base::AutoLock* lock, OpImpl&& impl);

    struct SnapshotObjects;

    bool doSnapshotOperation(const SnapshotObjects& objects,
                             SnapshotState operation);
    void waitForSnapshotCompletion(android::base::AutoLock* lock);
    void loadImpl(android::base::AutoLock* lock, const SnapshotObjects& objects);
    void saveImpl(android::base::AutoLock* lock, const SnapshotObjects& objects);

    bool isPausedForSnapshotLocked() const;

    RenderChannelImpl* mChannel = nullptr;
    SnapshotState mState = SnapshotState::Empty;
    bool mFinished = false;
    android::base::Lock mLock;
    android::base::ConditionVariable mCondVar;
    android::base::Optional<android::base::MemStream> mStream;
};

}  // namespace emugl
