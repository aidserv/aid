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
#include "WindowSurface.h"

#include "ErrorLog.h"
#include "FbConfig.h"
#include "FrameBuffer.h"

#include "OpenGLESDispatch/EGLDispatch.h"

#include <assert.h>
#include <GLES/glext.h>
#include <stdio.h>
#include <string.h>


WindowSurface::WindowSurface(EGLDisplay display,
                             EGLConfig config,
                             HandleType hndl) :
        mConfig(config),
        mDisplay(display),
        mHndl(hndl) {}

WindowSurface::~WindowSurface() {
    if (mSurface) {
        s_egl.eglDestroySurface(mDisplay, mSurface);
    }
}

WindowSurface *WindowSurface::create(EGLDisplay display,
                                     EGLConfig config,
                                     int p_width,
                                     int p_height,
                                     HandleType hndl) {
    // allocate space for the WindowSurface object
    WindowSurface *win = new WindowSurface(display, config, hndl);
    if (!win) {
        return NULL;
    }

    // Create a pbuffer to be used as the egl surface
    // for that window.
    if (!win->resize(p_width, p_height)) {
        delete win;
        return NULL;
    }

    return win;
}


void WindowSurface::setColorBuffer(ColorBufferPtr p_colorBuffer) {
    mAttachedColorBuffer = p_colorBuffer;
    if (!p_colorBuffer) return;

    // resize the window if the attached color buffer is of different
    // size.
    unsigned int cbWidth = mAttachedColorBuffer->getWidth();
    unsigned int cbHeight = mAttachedColorBuffer->getHeight();

    if (cbWidth != mWidth || cbHeight != mHeight) {
        resize(cbWidth, cbHeight);
    }
}

void WindowSurface::bind(RenderContextPtr p_ctx, BindType p_bindType) {
    if (p_bindType == BIND_READ) {
        mReadContext = p_ctx;
    } else if (p_bindType == BIND_DRAW) {
        mDrawContext = p_ctx;
    } else if (p_bindType == BIND_READDRAW) {
        mReadContext = p_ctx;
        mDrawContext = p_ctx;
    }
}

GLuint WindowSurface::getWidth() const { return mWidth; }
GLuint WindowSurface::getHeight() const { return mHeight; }

bool WindowSurface::flushColorBuffer() {
    if (!mAttachedColorBuffer.get()) {
        return true;
    }
    if (!mWidth || !mHeight) {
        return false;
    }

    if (mAttachedColorBuffer->getWidth() != mWidth ||
        mAttachedColorBuffer->getHeight() != mHeight) {
        // XXX: should never happen - how this needs to be handled?
        fprintf(stderr, "Dimensions do not match\n");
        return false;
    }

    if (!mDrawContext.get()) {
        fprintf(stderr, "Draw context is NULL\n");
        return false;
    }

    // Make the surface current
    EGLContext prevContext = s_egl.eglGetCurrentContext();
    EGLSurface prevReadSurf = s_egl.eglGetCurrentSurface(EGL_READ);
    EGLSurface prevDrawSurf = s_egl.eglGetCurrentSurface(EGL_DRAW);

    const bool needToSet = prevContext != mDrawContext->getEGLContext() ||
                           prevReadSurf != mSurface || prevDrawSurf != mSurface;
    if (needToSet) {
        if (!s_egl.eglMakeCurrent(mDisplay,
                                  mSurface,
                                  mSurface,
                                  mDrawContext->getEGLContext())) {
            fprintf(stderr, "Error making draw context current\n");
            return false;
        }
    }

    mAttachedColorBuffer->blitFromCurrentReadBuffer();

    if (needToSet) {
        // restore current context/surface
        s_egl.eglMakeCurrent(mDisplay, prevDrawSurf, prevReadSurf, prevContext);
    }

    return true;
}

bool WindowSurface::resize(unsigned int p_width, unsigned int p_height)
{
    if (mSurface && mWidth == p_width && mHeight == p_height) {
        // no need to resize
        return true;
    }

    EGLContext prevContext = s_egl.eglGetCurrentContext();
    EGLSurface prevReadSurf = s_egl.eglGetCurrentSurface(EGL_READ);
    EGLSurface prevDrawSurf = s_egl.eglGetCurrentSurface(EGL_DRAW);
    EGLSurface prevPbuf = mSurface;
    bool needRebindContext = mSurface &&
                             (prevReadSurf == mSurface ||
                              prevDrawSurf == mSurface);

    if (needRebindContext) {
        s_egl.eglMakeCurrent(
                mDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    }

    //
    // Destroy previous surface
    //
    if (mSurface) {
        s_egl.eglDestroySurface(mDisplay, mSurface);
        mSurface = NULL;
    }

    //
    // Create pbuffer surface.
    //
    const EGLint pbufAttribs[5] = {
        EGL_WIDTH, (EGLint) p_width, EGL_HEIGHT, (EGLint) p_height, EGL_NONE,
    };

    mSurface = s_egl.eglCreatePbufferSurface(mDisplay,
                                             mConfig,
                                             pbufAttribs);
    if (mSurface == EGL_NO_SURFACE) {
        fprintf(stderr, "Renderer error: failed to create/resize pbuffer!!\n");
        return false;
    }

    mWidth = p_width;
    mHeight = p_height;

    if (needRebindContext) {
        s_egl.eglMakeCurrent(
                mDisplay,
                (prevDrawSurf == prevPbuf) ? mSurface : prevDrawSurf,
                (prevReadSurf == prevPbuf) ? mSurface : prevReadSurf,
                prevContext);
    }

    return true;
}

HandleType WindowSurface::getHndl() const {
    return mHndl;
}

template <class obj_t>
static void saveHndlOrNull(obj_t obj, android::base::Stream* stream) {
    if (obj) {
        stream->putBe32(obj->getHndl());
    } else {
        stream->putBe32(0);
    }
}

void WindowSurface::onSave(android::base::Stream* stream) const {
    stream->putBe32(getHndl());
    saveHndlOrNull(mAttachedColorBuffer, stream);
    saveHndlOrNull(mReadContext, stream);
    saveHndlOrNull(mDrawContext, stream);
    stream->putBe32(mWidth);
    stream->putBe32(mHeight);
    if (s_egl.eglSaveConfig) {
        s_egl.eglSaveConfig(mDisplay, mConfig, stream);
    }
}

WindowSurface * WindowSurface::onLoad(android::base::Stream* stream,
            EGLDisplay display) {
    FrameBuffer* fb = FrameBuffer::getFB();
    HandleType hndl = stream->getBe32();
    HandleType cb = stream->getBe32();
    HandleType readCtx = stream->getBe32();
    HandleType drawCtx = stream->getBe32();

    GLuint width = stream->getBe32();
    GLuint height = stream->getBe32();
    EGLConfig config = 0;
    if (s_egl.eglLoadConfig) {
        config = s_egl.eglLoadConfig(display, stream);
    }
    WindowSurface* ret = create(display, config, width, height, hndl);
    assert(ret);
    // fb is already locked by its caller
    ret->mAttachedColorBuffer = fb->getColorBuffer_locked(cb);
    assert(!cb || ret->mAttachedColorBuffer);
    ret->mReadContext = fb->getContext_locked(readCtx);
    ret->mDrawContext = fb->getContext_locked(drawCtx);
    return ret;
}
