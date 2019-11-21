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

#include "OpenglRender/Renderer.h"

#include "RenderWindow.h"

#include "android/base/Compiler.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/synchronization/MessageChannel.h"
#include "android/base/threads/FunctorThread.h"

#include "RenderThread.h"

#include <memory>
#include <utility>
#include <vector>

namespace emugl {

class RendererImpl final : public Renderer {
public:
    RendererImpl();
    virtual ~RendererImpl();

    bool initialize(int width, int height, bool useSubWindow);
    void stop();

public:
    RenderChannelPtr createRenderChannel(android::base::Stream* loadStream) final;
    HardwareStrings getHardwareStrings() final;
    void setPostCallback(OnPostCallback onPost,
                                 void* context) final;
    bool showOpenGLSubwindow(FBNativeWindowType window,
                                     int wx,
                                     int wy,
                                     int ww,
                                     int wh,
                                     int fbw,
                                     int fbh,
                                     float dpr,
                                     float zRot) final;
    bool destroyOpenGLSubwindow() final;
    void setOpenGLDisplayRotation(float zRot) final;
    void setOpenGLDisplayTranslation(float px, float py) final;
    void repaintOpenGLDisplay() final;
    void cleanupProcGLObjects(uint64_t puid) final;

    void pauseAllPreSave() final;
    void resumeAll() final;

    void save(android::base::Stream* stream) final;
    bool load(android::base::Stream* stream) final;

private:
    DISALLOW_COPY_ASSIGN_AND_MOVE(RendererImpl);

private:
    // Stop all render threads and wait until they exit.
    void cleanupRenderThreads();
    // Wait for process cleanup to complete.
    void waitForProcessCleanup();

    std::unique_ptr<RenderWindow> mRenderWindow;

    android::base::Lock mChannelsLock;

    std::vector<std::shared_ptr<RenderChannelImpl>> mChannels;
    bool mStopped = false;

    class ProcessCleanupThread;
    std::unique_ptr<ProcessCleanupThread> mCleanupThread;
};

}  // namespace emugl
