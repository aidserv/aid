// Copyright 2016 The Android Open Source Project
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include "OpenGLESDispatch/GLESv2Dispatch.h"

// This is a 2D RGB texture that can be rendered to.
class GLCanvas {
public:
    GLCanvas(int width, int height, const GLESv2Dispatch* gl_dispatch);
    ~GLCanvas();

    // Width of the texture in pixels.
    int width() const { return mWidth; }

    // Height of the textrure in pixels.
    int height() const { return mHeight; }

    // Returns the underlying OpenGL texture object.
    GLuint texture() const { return mTargetTexture; }

    // All rendering operations following a call to bind()
    // be performed on the underlying texture, until unbind() is
    // called on the same instance, or bind() is called on some
    // other GLCanvas instance.
    void bind();

    // Unbind the canvas.
    void unbind();

private:
    int mWidth;
    int mHeight;
    const GLESv2Dispatch* mGLES2;
    GLuint mFramebuffer;
    GLuint mTargetTexture;
    GLuint mDepthBuffer;
};
