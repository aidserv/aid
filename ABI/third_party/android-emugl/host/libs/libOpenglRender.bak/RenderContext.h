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
#ifndef _LIBRENDER_RENDER_CONTEXT_H
#define _LIBRENDER_RENDER_CONTEXT_H

// #include "android/base/files/Stream.h"
#include "emugl/common/smart_ptr.h"
#include "GLDecoderContextData.h"

#include <EGL/egl.h>

// Type of handles, a.k.a. "object names" in the GL specification.
// These are integers used to uniquely identify a resource of a given type.
typedef uint32_t HandleType;

// Tracks all the possible OpenGL ES API versions.
enum GLESApi {
    GLESApi_CM = 1,
    GLESApi_2 = 2,
    GLESApi_3_0 = 3,
    GLESApi_3_1 = 4,
    GLESApi_3_2 = 5,
};

// A class used to model a guest EGLContext. This simply wraps a host
// EGLContext, associated with an GLDecoderContextData instance that is
// used to store copies of guest-side arrays.
class RenderContext {
public:
    // Create a new RenderContext instance.
    // |display| is the host EGLDisplay handle.
    // |config| is the host EGLConfig to use.
    // |sharedContext| is either EGL_NO_CONTEXT of a host EGLContext handle.
    // |version| specifies the GLES version as a GLESApi.
    static RenderContext *create(EGLDisplay display,
                                 EGLConfig config,
                                 EGLContext sharedContext,
                                 HandleType hndl,
                                 GLESApi = GLESApi_CM);

    // Destructor.
    ~RenderContext();

    // Retrieve host EGLContext value.
    EGLContext getEGLContext() const { return mContext; }

    // Retrieve emulated GLES1 context.
    void* getEmulatedGLES1Context() const { return mEmulatedGLES1Context; }

    // Return the actual underlying version used in this context.
    GLESApi version() const { return mVersion; }

    // Retrieve GLDecoderContextData instance reference for this
    // RenderContext instance.
    GLDecoderContextData& decoderContextData() { return mContextData; }

    HandleType getHndl() const { return mHndl; }
#if 0
    void onSave(android::base::Stream* stream);
    static RenderContext *onLoad(android::base::Stream* stream,
            EGLDisplay display);
#endif // remove 快照
private:
    RenderContext();

    RenderContext(EGLDisplay display,
                  EGLContext context,
                  HandleType hndl,
                  GLESApi version,
                  void* emulatedGLES1Context);

    // Implementation of create
    // |stream| is the stream to load from when restoring a snapshot,
    // set |stream| to nullptr if it is not loading from a snapshot
    static RenderContext *createImpl(EGLDisplay display,
                                 EGLConfig config,
                                 EGLContext sharedContext,
                                 HandleType hndl,
                                 GLESApi version,
                                 android::base::Stream *stream);
private:
    EGLDisplay mDisplay;
    EGLContext mContext;
    HandleType mHndl;
    GLESApi mVersion;
    void* mEmulatedGLES1Context;
    GLDecoderContextData mContextData;
};

typedef emugl::SmartPtr<RenderContext> RenderContextPtr;

#endif  // _LIBRENDER_RENDER_CONTEXT_H
