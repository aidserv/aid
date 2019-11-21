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
#ifndef _LIBRENDER_COLORBUFFER_H
#define _LIBRENDER_COLORBUFFER_H

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES/gl.h>
//#include "android/base/files/Stream.h"
#include "emugl/common/smart_ptr.h"
#include "RenderContext.h"

#include <memory>

#include "FrameworkFormats.h"

class TextureDraw;
class TextureResize;
class YUVConverter;

// A class used to model a guest color buffer, and used to implement several
// related things:
//
//  - Every gralloc native buffer with HW read or write requirements will
//    allocate a host ColorBuffer instance. When gralloc_lock() is called,
//    the guest will use ColorBuffer::readPixels() to read the current content
//    of the buffer. When gralloc_unlock() is later called, it will call
//    ColorBuffer::subUpdate() to send the updated pixels.
//
//  - Every guest window EGLSurface is implemented by a host PBuffer
//    (see WindowSurface.h) that can have a ColorBuffer instance attached to
//    it (through WindowSurface::attachColorBuffer()). When such an attachment
//    exists, WindowSurface::flushColorBuffer() will copy the PBuffer's
//    pixel data into the ColorBuffer. The latter can then be displayed
//    in the client's UI sub-window with ColorBuffer::post().
//
//  - Guest EGLImages are implemented as native gralloc buffers too.
//    The guest glEGLImageTargetTexture2DOES() implementations will end up
//    calling ColorBuffer::bindToTexture() to bind the current context's
//    GL_TEXTURE_2D to the buffer. Similarly, the guest versions of
//    glEGLImageTargetRenderbufferStorageOES() will end up calling
//    ColorBuffer::bindToRenderbuffer().
//
// This forces the implementation to use a host EGLImage to implement each
// ColorBuffer.
//
// As an additional twist.

class ColorBuffer {
public:
    // Helper interface class used during ColorBuffer operations. This is
    // introduced to remove coupling from the FrameBuffer class implementation.
    class Helper {
    public:
        Helper() {}
        virtual ~Helper() {}
        virtual bool setupContext() = 0;
        virtual void teardownContext() = 0;
        virtual TextureDraw* getTextureDraw() const = 0;
        virtual bool isBound() const = 0;
    };

    // Helper class to use a ColorBuffer::Helper context.
    // Usage is pretty simple:
    //
    //     {
    //        RecursiveScopedHelperContext context(m_helper);
    //        if (!context.isOk()) {
    //            return false;   // something bad happened.
    //        }
    //        .... do something ....
    //     }   // automatically calls m_helper->teardownContext();
    //
    class RecursiveScopedHelperContext {
    public:
        RecursiveScopedHelperContext(ColorBuffer::Helper* helper) : mHelper(helper) {
            if (helper->isBound()) return;
            if (!helper->setupContext()) {
                mHelper = NULL;
                return;
            }
            mNeedUnbind = true;
        }

        bool isOk() const { return mHelper != NULL; }

        ~RecursiveScopedHelperContext() { release(); }

        void release() {
            if (mNeedUnbind) {
                mHelper->teardownContext();
                mNeedUnbind = false;
            }
            mHelper = NULL;
        }

    private:
        ColorBuffer::Helper* mHelper;
        bool mNeedUnbind = false;
    };

    // Create a new ColorBuffer instance.
    // |p_display| is the host EGLDisplay handle.
    // |p_width| and |p_height| are the buffer's dimensions in pixels.
    // |p_internalFormat| is the internal OpenGL pixel format to use, valid
    // values
    // are: GL_RGB, GL_RGB565, GL_RGBA, GL_RGB5_A1_OES and GL_RGBA4_OES.
    // Implementation is free to use something else though.
    // |p_frameworkFormat| specifies the original format of the guest
    // color buffer so that we know how to convert to |p_internalFormat|,
    // if necessary (otherwise, p_frameworkFormat ==
    // FRAMEWORK_FORMAT_GL_COMPATIBLE).
    // |has_eglimage_texture_2d| should be true iff the display supports
    // the EGL_KHR_gl_texture_2D_image extension.
    // Returns NULL on failure.
    static ColorBuffer* create(EGLDisplay p_display,
                               int p_width,
                               int p_height,
                               GLenum p_internalFormat,
                               FrameworkFormat p_frameworkFormat,
                               bool has_eglimage_texture_2d,
                               HandleType hndl,
                               Helper* helper);

    // Destructor.
    ~ColorBuffer();

    // Return ColorBuffer width and height in pixels
    GLuint getWidth() const { return m_width; }
    GLuint getHeight() const { return m_height; }

    // Read the ColorBuffer instance's pixel values into host memory.
    void readPixels(int x,
                    int y,
                    int width,
                    int height,
                    GLenum p_format,
                    GLenum p_type,
                    void* pixels);

    // Update the ColorBuffer instance's pixel values from host memory.
    // |p_format / p_type| are the desired OpenGL color buffer format
    // and data type.
    // Otherwise, subUpdate() will explicitly convert |pixels|
    // to be in |p_format|.
    void subUpdate(int x,
                   int y,
                   int width,
                   int height,
                   GLenum p_format,
                   GLenum p_type,
                   void* pixels);

    // Draw a ColorBuffer instance, i.e. blit it to the current guest
    // framebuffer object / window surface. This doesn't display anything.
    bool draw();

    // Scale the underlying texture of this ColorBuffer to match viewport size.
    // It returns the texture name after scaling.
    GLuint scale();
    // Post this ColorBuffer to the host native sub-window.
    // |rotation| is the rotation angle in degrees, clockwise in the GL
    // coordinate space.
    bool post(GLuint tex, float rotation, float dx, float dy);

    // Bind the current context's EGL_TEXTURE_2D texture to this ColorBuffer's
    // EGLImage. This is intended to implement glEGLImageTargetTexture2DOES()
    // for all GLES versions.
    bool bindToTexture();

    // Bind the current context's EGL_RENDERBUFFER_OES render buffer to this
    // ColorBuffer's EGLImage. This is intended to implement
    // glEGLImageTargetRenderbufferStorageOES() for all GLES versions.
    bool bindToRenderbuffer();

    // Copy the content of the current context's read surface to this
    // ColorBuffer. This is used from WindowSurface::flushColorBuffer().
    // Return true on success, false on failure (e.g. no current context).
    bool blitFromCurrentReadBuffer();

    // Read the content of the whole ColorBuffer as 32-bit RGBA pixels.
    // |img| must be a buffer large enough (i.e. width * height * 4).
    void readback(unsigned char* img);
#if 0
    void onSave(android::base::Stream* stream);
    static ColorBuffer* onLoad(android::base::Stream* stream,
                               EGLDisplay p_display,
                               bool has_eglimage_texture_2d,
                               Helper* helper);
#endif
    HandleType getHndl() const;
private:
    ColorBuffer(EGLDisplay display, HandleType hndl, Helper* helper);

private:
    GLuint m_tex = 0;
    GLuint m_blitTex = 0;
    EGLImageKHR m_eglImage = nullptr;
    EGLImageKHR m_blitEGLImage = nullptr;
    GLuint m_width = 0;
    GLuint m_height = 0;
    GLuint m_fbo = 0;
    GLenum m_internalFormat = 0;
    EGLDisplay m_display = nullptr;
    Helper* m_helper = nullptr;
    TextureResize* m_resizer = nullptr;
    FrameworkFormat m_frameworkFormat;
    GLuint m_yuv_conversion_fbo = 0;  // FBO to offscreen-convert YUV to RGB
    std::unique_ptr<YUVConverter> m_yuv_converter;
    HandleType mHndl;
};

typedef emugl::SmartPtr<ColorBuffer> ColorBufferPtr;
//typedef std::shared_ptr<ColorBuffer> ColorBufferPtr;
#endif
