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
#include "RenderContext.h"

#include "OpenGLESDispatch/EGLDispatch.h"
#include "OpenGLESDispatch/GLESv1Dispatch.h"

#include <assert.h>
#include <OpenglCodecCommon/ErrorLog.h>

extern GLESv1Dispatch s_gles1;

RenderContext* RenderContext::create(EGLDisplay display,
                                     EGLConfig config,
                                     EGLContext sharedContext,
                                     HandleType hndl,
                                     GLESApi version) {
    return createImpl(display, config, sharedContext, hndl, version, nullptr);
}

RenderContext* RenderContext::createImpl(EGLDisplay display,
                                     EGLConfig config,
                                     EGLContext sharedContext,
                                     HandleType hndl,
                                     GLESApi version,
                                     void *stream) {
    void* emulatedGLES1Context = NULL;

    bool shouldEmulateGLES1 = s_gles1.underlying_gles2_api != NULL;

    GLESApi clientVersion = version;
    int majorVersion = clientVersion;
    int minorVersion = 0;

    if (version == GLESApi_CM && shouldEmulateGLES1) {
        clientVersion = GLESApi_2;
    } else if (version == GLESApi_3_0) {
        majorVersion = 3;
        minorVersion = 0;
    } else if (version == GLESApi_3_1) {
        majorVersion = 3;
        minorVersion = 1;
    }

    const EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, majorVersion,
        EGL_CONTEXT_MINOR_VERSION_KHR, minorVersion,
        EGL_NONE
    };
    EGLContext context;
    if (stream && s_egl.eglLoadContext) {
        context = s_egl.eglLoadContext(display, contextAttribs, stream);
    } else {
        context = s_egl.eglCreateContext(
            display, config, sharedContext, contextAttribs);
    }
    if (context == EGL_NO_CONTEXT) {
        fprintf(stderr, "%s: failed to create context (EGL_NO_CONTEXT result)\n", __func__);
        return NULL;
    }

    if (shouldEmulateGLES1) {
        DBG("%s: should be creating a emulated gles1 context here\n", __FUNCTION__);
        if (sharedContext == EGL_NO_CONTEXT) {
            DBG("%s: thankfully, this context isn't sharing anything :)\n", __FUNCTION__);
        } else {
            DBG("%s: this context is shared. need to maintain a sort of map I guess\n", __FUNCTION__);
        }
        emulatedGLES1Context = s_gles1.create_gles1_context(NULL, s_gles1.underlying_gles2_api);
        DBG("%s: created a emulated gles1 context @ %p\n", __FUNCTION__, emulatedGLES1Context);
        return new RenderContext(display, context, hndl, clientVersion, emulatedGLES1Context);
    } else {
        return new RenderContext(display, context, hndl, clientVersion, NULL);
    }
}

RenderContext::RenderContext(EGLDisplay display,
                             EGLContext context,
                             HandleType hndl,
                             GLESApi version,
                             void* emulatedGLES1Context) :
        mDisplay(display),
        mContext(context),
        mHndl(hndl),
        mVersion(version),
        mEmulatedGLES1Context(emulatedGLES1Context),
        mContextData() { }

RenderContext::~RenderContext() {
    if (mContext != EGL_NO_CONTEXT) {
        s_egl.eglDestroyContext(mDisplay, mContext);
    }
    if (mEmulatedGLES1Context != NULL) {
        s_gles1.destroy_gles1_context(mEmulatedGLES1Context);
        mEmulatedGLES1Context = NULL;
    }
}
#if 0
void RenderContext::onSave(android::base::Stream* stream) {
    stream->putBe32(mHndl);
    stream->putBe32(static_cast<uint32_t>(mVersion));
    assert(s_egl.eglCreateContext);
    if (s_egl.eglSaveContext) {
        s_egl.eglSaveContext(mDisplay, mContext, static_cast<EGLStream>(stream));
    }
}

RenderContext *RenderContext::onLoad(android::base::Stream* stream,
            EGLDisplay display) {
    HandleType hndl = static_cast<HandleType>(stream->getBe32());
    GLESApi version = static_cast<GLESApi>(stream->getBe32());

    return createImpl(display, (EGLConfig)0, EGL_NO_CONTEXT, hndl, version,
                      stream);
}
#endif
