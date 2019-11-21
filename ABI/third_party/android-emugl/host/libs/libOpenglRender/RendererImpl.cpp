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
#include "RendererImpl.h"

//#include "RenderChannelImpl.h"

#include "emugl/common/logging.h"
#include "ErrorLog.h"
#include "FenceSync.h"
#include "FrameBuffer.h"

#include <algorithm>
#include <utility>

#include <assert.h>

namespace emugl {

// kUseSubwindowThread is used to determine whether the RenderWindow should use
// a separate thread to manage its subwindow GL/GLES context.
// For now, this feature is disabled entirely for the following
// reasons:
//
// - It must be disabled on Windows at all times, otherwise the main window
//   becomes unresponsive after a few seconds of user interaction (e.g. trying
//   to move it over the desktop). Probably due to the subtle issues around
//   input on this platform (input-queue is global, message-queue is
//   per-thread). Also, this messes considerably the display of the
//   main window when running the executable under Wine.
//
// - On Linux/XGL and OSX/Cocoa, this used to be necessary to avoid corruption
//   issues with the GL state of the main window when using the SDL UI.
//   After the switch to Qt, this is no longer necessary and may actually cause
//   undesired interactions between the UI thread and the RenderWindow thread:
//   for example, in a multi-monitor setup the context might be recreated when
//   dragging the window between monitors, triggering a Qt-specific callback
//   in the context of RenderWindow thread, which will become blocked on the UI
//   thread, which may in turn be blocked on something else.
static const bool kUseSubwindowThread = false;
#if 0
// This object manages the cleanup of guest process resources when the process
// exits. It runs the cleanup in a separate thread to never block the main
// render thread for a low-priority task.
class RendererImpl::ProcessCleanupThread {
public:
    ProcessCleanupThread()
        : mCleanupThread([this]() {
              while (const auto id = mCleanupProcessIds.receive()) {
                  FrameBuffer::getFB()->cleanupProcGLObjects(*id);
              }
          }) {
        mCleanupThread.start();
    }

    ~ProcessCleanupThread() {
        mCleanupProcessIds.stop();
        mCleanupThread.wait();
    }

    void cleanup(uint64_t processId) {
        mCleanupProcessIds.send(processId);
    }

    void waitForCleanup() {
        mCleanupProcessIds.waitForEmpty();
    }

private:
    DISALLOW_COPY_AND_ASSIGN(ProcessCleanupThread);

    android::base::MessageChannel<uint64_t, 64> mCleanupProcessIds;
    android::base::FunctorThread mCleanupThread;
};
#endif
RendererImpl::RendererImpl() {
    // mCleanupThread.reset(new ProcessCleanupThread());
    mRenderWindow = NULL;
}

RendererImpl::~RendererImpl() {
    stop();
    // mRenderWindow.reset();
    if (mRenderWindow != NULL)
	{
		delete mRenderWindow;
    	mRenderWindow = NULL;
	}
}

bool RendererImpl::initialize(int width, int height, bool useSubWindow) {
    if (mRenderWindow) {
        ERR("mRenderWindow != NULL\n");
        return false;
    }
	#if 0
    std::unique_ptr<RenderWindow> renderWindow(
            new RenderWindow(width, height, kUseSubwindowThread, useSubWindow));
    #else
    RenderWindow *renderWindow = new RenderWindow(width, height, kUseSubwindowThread, useSubWindow);
    #endif
    if (!renderWindow) {
        ERR("Could not create rendering window class\n");
        GL_LOG("Could not create rendering window class");
        return false;
    }
    if (!renderWindow->isValid()) {
        ERR("Could not initialize emulated framebuffer\n");
        return false;
    }

    mRenderWindow = renderWindow; // std::move(renderWindow);
    GL_LOG("OpenGL renderer initialized successfully");
    return true;
}

void RendererImpl::stop() {
#if 0
    android::base::AutoLock lock(mChannelsLock);
    mStopped = true;
    auto channels = std::move(mChannels);
    lock.unlock();
#endif
    if (const auto fb = FrameBuffer::getFB()) {
        fb->setShuttingDown();
    }
#if 0
    for (const auto& c : channels) {
        c->stopFromHost();
    }
    // Each render channel is referenced in the corresponing pipe object, so
    // even if we clear the |channels| vector they could still be alive
    // for a while. This means we need to make sure to wait for render thread
    // exit explicitly.
    for (const auto& c : channels) {
        c->renderThread()->wait();
    }
#endif
    // We're stopping the renderer, so there's no need to clean up resources
    // of some pending processes: we'll destroy everything soon.
    // mCleanupThread.reset();
}

#if 0
void RendererImpl::cleanupRenderThreads() {
    android::base::AutoLock lock(mChannelsLock);
    const auto channels = std::move(mChannels);
    assert(mChannels.empty());
    lock.unlock();
    for (const auto& c : channels) {
        // Please DO NOT notify the guest about this event (DO NOT call
        // stopFromHost() ), because this is used to kill old threads when
        // loading from a snapshot, and the newly loaded guest should not
        // be notified for those behavior.
        c->stop();
    }
    for (const auto& c : channels) {
        c->renderThread()->wait();
    }
}

void RendererImpl::waitForProcessCleanup() {
    mCleanupThread->waitForCleanup();
    // Recreate it to make sure we've started from scratch and that we've
    // finished all in-progress cleanups as well.
    mCleanupThread.reset(new ProcessCleanupThread());
}
RenderChannelPtr RendererImpl::createRenderChannel(
        android::base::Stream* loadStream) {
    printf("createRenderChannel\n");
    const auto channel = std::make_shared<RenderChannelImpl>(loadStream);
    {
        android::base::AutoLock lock(mChannelsLock);

        if (mStopped) {
            return nullptr;
        }

        // Clean up the stopped channels.
        mChannels.erase(
                std::remove_if(mChannels.begin(), mChannels.end(),
                               [](const std::shared_ptr<RenderChannelImpl>& c) {
                                   return c->renderThread()->isFinished();
                               }),
                mChannels.end());
        mChannels.emplace_back(channel);

        DBG("Started new RenderThread (total %" PRIu64 ") @%p\n",
            static_cast<uint64_t>(mChannels.size()), channel->renderThread());
    }

    return channel;
}

void RendererImpl::pauseAllPreSave() {
    android::base::AutoLock lock(mChannelsLock);
    if (mStopped) {
        return;
    }
    for (const auto& c : mChannels) {
        c->renderThread()->pausePreSnapshot();
    }
    lock.unlock();
    waitForProcessCleanup();
}

void RendererImpl::resumeAll() {
    {
        android::base::AutoLock lock(mChannelsLock);
        if (mStopped) {
            return;
        }
        for (const auto& c : mChannels) {
            c->renderThread()->resume();
        }
    }

    repaintOpenGLDisplay();
}

void RendererImpl::save(android::base::Stream* stream) {
    stream->putByte(mStopped);
    if (mStopped) {
        return;
    }
    auto fb = FrameBuffer::getFB();
    assert(fb);
    fb->onSave(stream);

    FenceSync::onSave(stream);
}

bool RendererImpl::load(android::base::Stream* stream) {
#ifdef SNAPSHOT_PROFILE
    android::base::System::Duration startTime
            = android::base::System::get()->getUnixTimeUs();
#endif
    cleanupRenderThreads();
    waitForProcessCleanup();
#ifdef SNAPSHOT_PROFILE
    printf("RenderThread cleanup time: %ld ms\n",
            (android::base::System::get()->getUnixTimeUs()
            - startTime) / 1000);
#endif

    mStopped = stream->getByte();
    if (mStopped) {
        return true;
    }
    auto fb = FrameBuffer::getFB();
    assert(fb);

    bool res = true;

    res = fb->onLoad(stream);
    FenceSync::onLoad(stream);

    return res;
}
#endif // end remove
RendererImpl::HardwareStrings RendererImpl::getHardwareStrings() {
    assert(mRenderWindow);

    const char* vendor = nullptr;
    const char* renderer = nullptr;
    const char* version = nullptr;
    if (!mRenderWindow->getHardwareStrings(&vendor, &renderer, &version)) {
        return {};
    }
    HardwareStrings res;
    res.vendor = vendor ? vendor : "";
    res.renderer = renderer ? renderer : "";
    res.version = version ? version : "";
    return res;
}

void RendererImpl::setPostCallback(RendererImpl::OnPostCallback onPost,
                                   void* context) {
    assert(mRenderWindow);
    mRenderWindow->setPostCallback(onPost, context);
}

bool RendererImpl::showOpenGLSubwindow(FBNativeWindowType window,
                                       int wx,
                                       int wy,
                                       int ww,
                                       int wh,
                                       int fbw,
                                       int fbh,
                                       float dpr,
                                       float zRot) {
    assert(mRenderWindow);
    return mRenderWindow->setupSubWindow(window, wx, wy, ww, wh, fbw, fbh, dpr,
                                         zRot);
}

bool RendererImpl::destroyOpenGLSubwindow() {
    assert(mRenderWindow);
    return mRenderWindow->removeSubWindow();
}

void RendererImpl::setOpenGLDisplayRotation(float zRot) {
    assert(mRenderWindow);
    mRenderWindow->setRotation(zRot);
}

void RendererImpl::setOpenGLDisplayTranslation(float px, float py) {
    assert(mRenderWindow);
    mRenderWindow->setTranslation(px, py);
}

void RendererImpl::repaintOpenGLDisplay() {
    assert(mRenderWindow);
    mRenderWindow->repaint();
}

void RendererImpl::cleanupProcGLObjects(uint64_t puid) {
    // mCleanupThread->cleanup(puid);
}

}  // namespace emugl
