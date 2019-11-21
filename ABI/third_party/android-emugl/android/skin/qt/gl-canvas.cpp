// Copyright 2016 The Android Open Source Project
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/gl-canvas.h"

#include "android/skin/qt/gl-common.h"
#include "GLES2/gl2.h"

#include <cassert>

GLCanvas::GLCanvas(int w, int h, const GLESv2Dispatch* gl_dispatch) :
        // Note that width and height must be powers of 2 to comply with
        // GLES 2.0
        mWidth(w * 2),
        mHeight(h * 2),
        mGLES2(gl_dispatch),
        mFramebuffer(0),
        mTargetTexture(0),
        mDepthBuffer(0) {
    assert(((mWidth & (mWidth - 1)) == 0) && ((mHeight & (mHeight - 1)) == 0));
    // Create and bind framebuffer object.
    mGLES2->glGenFramebuffers(1, &mFramebuffer);
    CHECK_GL_ERROR("Failed to create framebuffer object");
    mGLES2->glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);
    CHECK_GL_ERROR("Failed to bind framebuffer object");

    // Create a texture to render to.
    mGLES2->glGenTextures(1, &mTargetTexture);
    CHECK_GL_ERROR("Failed to create target texture for FBO");
    mGLES2->glBindTexture(GL_TEXTURE_2D, mTargetTexture);
    mGLES2->glTexImage2D(GL_TEXTURE_2D,
                         0, // mipmap level 0
                         GL_RGB,
                         width(),
                         height(),
                         0, // border, must be 0
                         GL_RGB,
                         GL_UNSIGNED_BYTE,
                         0); // don't upload any data, just allocate space
    CHECK_GL_ERROR("Failed to populate target texture");
    mGLES2->glTexParameteri(GL_TEXTURE_2D,
                            GL_TEXTURE_WRAP_S,
                            GL_CLAMP_TO_EDGE);
    mGLES2->glTexParameteri(GL_TEXTURE_2D,
                            GL_TEXTURE_WRAP_T,
                            GL_CLAMP_TO_EDGE);
    mGLES2->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    mGLES2->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    mGLES2->glFramebufferTexture2D(GL_FRAMEBUFFER,
                                   GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_2D,
                                   mTargetTexture,
                                   0); // mipmap level, must be 0
    CHECK_GL_ERROR("Failed to set FBO color attachment");

    // Create a depth buffer for the FBO.
    mGLES2->glGenRenderbuffers(1, &mDepthBuffer);
    CHECK_GL_ERROR("Failed to create depth buffer");
    mGLES2->glBindRenderbuffer(GL_RENDERBUFFER, mDepthBuffer);
    mGLES2->glRenderbufferStorage(GL_RENDERBUFFER,
                                  GL_DEPTH_COMPONENT16,
                                  width(),
                                  height());
    CHECK_GL_ERROR("Failed to initialize depth buffer");
    mGLES2->glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                                      GL_DEPTH_ATTACHMENT,
                                      GL_RENDERBUFFER,
                                      mDepthBuffer);
    CHECK_GL_ERROR("Failed to attach depth buffer to FBO");
    mGLES2->glBindRenderbuffer(GL_RENDERBUFFER, 0);

    // Verify that FBO is valid.
    GLenum fb_status = mGLES2->glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (fb_status != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "FBO not complete");
    }

    // Restore the default frame-buffer.
    mGLES2->glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLCanvas::~GLCanvas() {
    if (mFramebuffer) {
        mGLES2->glDeleteFramebuffers(1, &mFramebuffer);
        mFramebuffer = 0;
    }

    if (mTargetTexture) {
        mGLES2->glDeleteTextures(1, &mTargetTexture);
        mTargetTexture = 0;
    }

    if (mDepthBuffer) {
        mGLES2->glDeleteRenderbuffers(1, &mDepthBuffer);
        mDepthBuffer = 0;
    }
}

void GLCanvas::bind() {
    mGLES2->glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);
    mGLES2->glViewport(0, 0, width(), height());
}

void GLCanvas::unbind() {
    mGLES2->glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
