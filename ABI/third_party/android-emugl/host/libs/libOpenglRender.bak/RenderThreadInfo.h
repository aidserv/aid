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
#ifndef _LIB_OPENGL_RENDER_THREAD_INFO_H
#define _LIB_OPENGL_RENDER_THREAD_INFO_H

#include "android/base/files/Stream.h"
#include "RenderContext.h"
#include "WindowSurface.h"
#include "GLESv1Decoder.h"
#include "GLESv2Decoder.h"
#include "renderControl_dec.h"
#include "StalePtrRegistry.h"
#include "SyncThread.h"

#include <unordered_set>

typedef uint32_t HandleType;
typedef std::unordered_set<HandleType> ThreadContextSet;
typedef std::unordered_set<HandleType> WindowSurfaceSet;

// A class used to model the state of each RenderThread related
struct RenderThreadInfo {
    // Create new instance. Only call this once per thread.
    // Future callls to get() will return this instance until
    // it is destroyed.
    RenderThreadInfo();

    // Destructor.
    ~RenderThreadInfo();

    // Return the current thread's instance, if any, or NULL.
    static RenderThreadInfo* get();
    // Allocates |syncThread| and updates tracking.
    void createSyncThread();
    // The opposite.
    void destroySyncThread();

    // Current EGL context, draw surface and read surface.
    RenderContextPtr currContext;
    WindowSurfacePtr currDrawSurf;
    WindowSurfacePtr currReadSurf;

    // Decoder states.
    GLESv1Decoder                   m_glDec;
    GLESv2Decoder                   m_gl2Dec;
    renderControl_decoder_context_t m_rcDec;

    // All the contexts that are created by this render thread.
    // New emulator manages contexts in guest process level,
    // m_contextSet should be deprecated. It is only kept for
    // backward compatibility reason.
    ThreadContextSet                m_contextSet;
    // all the window surfaces that are created by this render thread
    WindowSurfaceSet                m_windowSet;

    // Sync timeline info + sync thread pointer
    std::unique_ptr<SyncThread>     syncThread;
    // Mapping to sync thread pointer value on snapshot save/load
    uint64_t syncThreadAlias = 0;

    // The unique id of owner guest process of this render thread
    uint64_t                        m_puid = 0;

    // Functions to save / load a snapshot
    // They must be called after Framebuffer snapshot
    void onSave(android::base::Stream* stream);
    bool onLoad(android::base::Stream* stream);

    static StalePtrRegistry<SyncThread>* getSyncThreadRegistry();
};

#endif
