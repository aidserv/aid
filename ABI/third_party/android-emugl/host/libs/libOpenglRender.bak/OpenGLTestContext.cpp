// Copyright (C) 2017 The Android Open Source Project
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

#include <gtest/gtest.h>

#include "OpenGLTestContext.h"

namespace gltest {

static bool sDisplayNeedsInit = true;

EGLDisplay getDisplay() {
    const EGLDispatch* egl = LazyLoadedEGLDispatch::get();
    EGLDisplay dpy = egl->eglGetDisplay(EGL_DEFAULT_DISPLAY);
    EXPECT_TRUE(dpy != EGL_NO_DISPLAY);

    if (sDisplayNeedsInit) {
        EGLint eglMaj, eglMin;
        EGLBoolean init_res = egl->eglInitialize(dpy, &eglMaj, &eglMin);
        EXPECT_TRUE(init_res != EGL_FALSE);
        sDisplayNeedsInit = false;
    }

    return dpy;
}

EGLConfig createConfig(EGLDisplay dpy, EGLint r, EGLint g, EGLint b, EGLint a, EGLint d, EGLint s, EGLint ms) {
    const EGLDispatch* egl = LazyLoadedEGLDispatch::get();
    const EGLint configAttribs[] = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE, r,
        EGL_GREEN_SIZE, g,
        EGL_BLUE_SIZE, b,
        EGL_ALPHA_SIZE, a,
        EGL_DEPTH_SIZE, d,
        EGL_STENCIL_SIZE, s,
        EGL_SAMPLES, ms,
        EGL_NONE
    };

    EGLint nConfigs;
    EGLConfig configOut;
    EGLBoolean chooseConfigResult =
        egl->eglChooseConfig(
                dpy, configAttribs,
                &configOut,
                1,
                &nConfigs);
    EXPECT_TRUE(chooseConfigResult != EGL_FALSE);
    EXPECT_TRUE(nConfigs > 0);
    return configOut;
}

EGLSurface pbufferSurface(EGLDisplay dpy, ::EGLConfig config, EGLint w, EGLint h) {
    const EGLDispatch* egl = LazyLoadedEGLDispatch::get();
    const EGLint pbufferAttribs[] = {
        EGL_WIDTH, w,
        EGL_HEIGHT, h,
        EGL_NONE,
    };

    EGLSurface pbuf =
        egl->eglCreatePbufferSurface(
            dpy, config, pbufferAttribs);

    EXPECT_TRUE(pbuf != EGL_NO_SURFACE);
    return pbuf;
}

EGLContext createContext(EGLDisplay dpy, ::EGLConfig config, EGLint maj, EGLint min) {
    const EGLDispatch* egl = LazyLoadedEGLDispatch::get();
    const EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    EGLContext cxt = egl->eglCreateContext(dpy, config, EGL_NO_CONTEXT, contextAttribs);
    EXPECT_TRUE(cxt != EGL_NO_CONTEXT);
    return cxt;
}

void destroyContext(EGLDisplay dpy, EGLContext cxt) {
    const EGLDispatch* egl = LazyLoadedEGLDispatch::get();
    EGLBoolean destroyContextRes =
        egl->eglDestroyContext(dpy, cxt);
    EXPECT_TRUE(destroyContextRes != GL_FALSE);
}

void destroySurface(EGLDisplay dpy, EGLSurface surface) {
    const EGLDispatch* egl = LazyLoadedEGLDispatch::get();
    EGLBoolean destroySurfaceRes =
        egl->eglDestroySurface(dpy, surface);
    EXPECT_TRUE(destroySurfaceRes != GL_FALSE);
}

void destroyDisplay(EGLDisplay dpy) {
    const EGLDispatch* egl = LazyLoadedEGLDispatch::get();
    EGLBoolean terminateRes =
        egl->eglTerminate(dpy);
    EXPECT_TRUE(terminateRes != GL_FALSE);
    sDisplayNeedsInit = true;
}

} // namespace gltest
