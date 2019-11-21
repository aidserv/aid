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

#include "android/base/Log.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/synchronization/MessageChannel.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/threads/Thread.h"

#include <gtest/gtest.h>

#include "OpenGLTestContext.h"
#include "GLESv2Decoder.h"
#include "GLSnapshot.h"

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES3/gl31.h>

#define AUTO_RUN_GL_UNITTESTS 0

#if AUTO_RUN_GL_UNITTESTS
#define GLTEST(x, y) TEST(x, y)
#else
#define GLTEST(x, y) TEST(x, DISABLED_##y)
#endif

namespace emugl {

using android::base::Lock;
using android::base::AutoLock;
using namespace gltest;

GLTEST(OpenGL, Dispatch) {
    const EGLDispatch* egl = LazyLoadedEGLDispatch::get();
    const GLESv2Dispatch* gl = LazyLoadedGLESv2Dispatch::get();
    EXPECT_TRUE(egl != nullptr);
    EXPECT_TRUE(gl != nullptr);
}

GLTEST(OpenGL, CreateContext) {
    const EGLDispatch* egl = LazyLoadedEGLDispatch::get();
    const GLESv2Dispatch* gl = LazyLoadedGLESv2Dispatch::get();
    EXPECT_TRUE(egl != nullptr);
    EXPECT_TRUE(gl != nullptr);

    EGLDisplay dpy = getDisplay();
    EGLConfig config = createConfig(dpy, 8, 8, 8, 8, 24, 8, 0);
    EGLSurface surf = pbufferSurface(dpy, config, 512, 512);
    EGLContext context = createContext(dpy, config, 2, 0);

    destroyContext(dpy, context);
    destroySurface(dpy, surf);
    destroyDisplay(dpy);
}

class SnapshotContext : public GLESv2Decoder{
public:
    SnapshotContext() : mSnap(LazyLoadedGLESv2Dispatch::get()) {
        mWidth = 512;
        mHeight = 512;
        mMajorVersion = 2;
        mMinorVersion = 0;

        this->m_snapshot = &mSnap;
        initGL(gles2_dispatch_get_proc_func, NULL);
        EXPECT_TRUE(this->glGenBuffers != nullptr);

        mEGL = LazyLoadedEGLDispatch::get();
        EXPECT_TRUE(mEGL != nullptr);
        mDpy = getDisplay();
        mConfig = createConfig(mDpy, 8, 8, 8, 8, 24, 8, 0);
        mSurf = pbufferSurface(mDpy, mConfig, mWidth, mHeight);
        mContext = createContext(mDpy, mConfig, mMajorVersion, mMinorVersion);

        mEGL->eglMakeCurrent(mDpy, mSurf, mSurf, mContext);
    }

    void doSnapshot() {
        mSnap.save();
        mEGL->eglMakeCurrent(mDpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        destroyContext(mDpy, mContext);
        destroySurface(mDpy, mSurf);
        mSurf = pbufferSurface(mDpy, mConfig, mWidth, mHeight);
        mContext = createContext(mDpy, mConfig, mMajorVersion, mMinorVersion);
        mEGL->eglMakeCurrent(mDpy, mSurf, mSurf, mContext);
        mSnap.restore();
    }

    ~SnapshotContext() {
        mEGL->eglMakeCurrent(mDpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        destroyContext(mDpy, mContext);
        destroySurface(mDpy, mSurf);
        destroyDisplay(mDpy);
    }

private:
    int mWidth;
    int mHeight;
    int mMajorVersion;
    int mMinorVersion;
    const EGLDispatch* mEGL;

    GLSnapshot::GLSnapshotState mSnap;
    EGLDisplay mDpy;
    EGLConfig mConfig;
    EGLSurface mSurf;
    EGLContext mContext;
};

GLTEST(OpenGL, Snapshot_ClearColor) {
    SnapshotContext gl;

    gl.glClearColor(0.0, 1.0, 0.0, 1.0);

    gl.doSnapshot();

    GLfloat restoredClearColor[4];

    gl.glGetFloatv(GL_COLOR_CLEAR_VALUE, restoredClearColor);

    EXPECT_TRUE(restoredClearColor[0] == 0.0);
    EXPECT_TRUE(restoredClearColor[1] == 1.0);
    EXPECT_TRUE(restoredClearColor[2] == 0.0);
    EXPECT_TRUE(restoredClearColor[3] == 1.0);
}

GLTEST(OpenGL, Snapshot_ActiveTexture) {
    SnapshotContext gl;

    gl.glActiveTexture(GL_TEXTURE4);

    gl.doSnapshot();

    GLint restoredActiveTexture;
    gl.glGetIntegerv(GL_ACTIVE_TEXTURE, &restoredActiveTexture);

    EXPECT_TRUE(restoredActiveTexture == GL_TEXTURE4);
}

GLTEST(OpenGL, Snapshot_DepthTest) {
    SnapshotContext gl;

    gl.glEnable(GL_DEPTH_TEST);
    gl.doSnapshot();

    EXPECT_TRUE(gl.glIsEnabled(GL_DEPTH_TEST) == GL_TRUE);
}

GLTEST(OpenGL, Snapshot_CreateShader0) {
    SnapshotContext gl;

    GLuint shader = gl.glCreateShader_dec(&gl, GL_VERTEX_SHADER);
    gl.doSnapshot();

    EXPECT_TRUE(gl.glIsShader(shader) == GL_TRUE);
}

}  // namespace emugl
