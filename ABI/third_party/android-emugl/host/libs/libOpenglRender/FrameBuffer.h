/*
* Copyright (C) 2011-2015 The Android Open Source Project
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
#ifndef _LIBRENDER_FRAMEBUFFER_H
#define _LIBRENDER_FRAMEBUFFER_H

//#include "android/base/files/Stream.h"

#include "ColorBuffer.h"
#include "emugl/common/mutex.h"
#include "FbConfig.h"
#include "RenderContext.h"
#include "TextureDraw.h"
#include "WindowSurface.h"

#include "OpenglRender/render_api.h"
#include "OpenglRender/Renderer.h"

#include <EGL/egl.h>

#include <unordered_map>
#include <unordered_set>

#include <stdint.h>

struct ColorBufferRef {
    ColorBufferPtr cb;
    uint32_t refcount;  // number of client-side references
    bool opened; // tracks whether opened at least once. In O+,
    // color buffers can be created/closed immediately,
    // but then registered (opened) afterwards
};

typedef std::unordered_map<HandleType, std::pair<WindowSurfacePtr, HandleType> > WindowSurfaceMap;
typedef std::unordered_set<HandleType> WindowSurfaceSet;
typedef std::unordered_map<uint64_t, WindowSurfaceSet> ProcOwnedWindowSurfaces;

typedef std::unordered_map<HandleType, RenderContextPtr> RenderContextMap;
typedef std::unordered_set<HandleType> RenderContextSet;
typedef std::unordered_map<uint64_t, RenderContextSet> ProcOwnedRenderContexts;

typedef std::unordered_map<HandleType, ColorBufferRef> ColorBufferMap;
typedef std::unordered_multiset<HandleType> ColorBufferSet;
typedef std::unordered_map<uint64_t, ColorBufferSet> ProcOwnedColorBuffers;

typedef std::unordered_set<HandleType> EGLImageSet;
typedef std::unordered_map<uint64_t, EGLImageSet> ProcOwnedEGLImages;

// A structure used to list the capabilities of the underlying EGL
// implementation that the FrameBuffer instance depends on.
// |has_eglimage_texture_2d| is true iff the EGL_KHR_gl_texture_2D_image
// extension is supported.
// |has_eglimage_renderbuffer| is true iff the EGL_KHR_gl_renderbuffer_image
// extension is supported.
// |eglMajor| and |eglMinor| are the major and minor version numbers of
// the underlying EGL implementation.
struct FrameBufferCaps {
    bool has_eglimage_texture_2d;
    bool has_eglimage_renderbuffer;
    EGLint eglMajor;
    EGLint eglMinor;
};

// The FrameBuffer class holds the global state of the emulation library on
// top of the underlying EGL/GLES implementation. It should probably be
// named "Display" instead of "FrameBuffer".
//
// There is only one global instance, that can be retrieved with getFB(),
// and which must be previously setup by calling initialize().
//
class FrameBuffer {
public:
    // Initialize the global instance.
    // |width| and |height| are the dimensions of the emulator GPU display
    // in pixels. |useSubWindow| is true to indicate that the caller
    // will use setupSubWindow() to let EmuGL display the GPU content in its
    // own sub-windows. If false, this means the caller will use
    // setPostCallback() instead to retrieve the content.
    // Returns true on success, false otherwise.
    static bool initialize(int width, int height, bool useSubWindow);

    // Setup a sub-window to display the content of the emulated GPU
    // on-top of an existing UI window. |p_window| is the platform-specific
    // parent window handle. |wx|, |wy|, |ww| and |wh| are the
    // dimensions in pixels of the sub-window, relative to the parent window's
    // coordinate. |fbw| and |fbh| are the dimensions used to initialize
    // the framebuffer, which may be different from the dimensions of the
    // sub-window (in which case scaling will be applied automatically).
    // |dpr| is the device pixel ratio of the monitor, which is needed for
    // proper panning on high-density displays (like retina)
    // |zRot| is a rotation angle in degrees, (clockwise in the Y-upwards GL
    // coordinate space).
    //
    // If a sub-window already exists, this function updates the subwindow
    // and framebuffer properties to match the given values.
    //
    // Return true on success, false otherwise.
    //
    // NOTE: This can return false for software-only EGL engines like OSMesa.
    bool setupSubWindow(FBNativeWindowType p_window,
                        int wx, int wy,
                        int ww, int wh,
                        int fbw, int fbh, float dpr, float zRot);

    // Remove the sub-window created by setupSubWindow(), if any.
    // Return true on success, false otherwise.
    bool removeSubWindow();

    // Finalize the instance.
    void finalize();

    // Return a pointer to the global instance. initialize() must be called
    // previously, or this will return NULL.
    static FrameBuffer *getFB() { return s_theFrameBuffer; }

    // Return the capabilities of the underlying display.
    const FrameBufferCaps &getCaps() const { return m_caps; }

    // Return the emulated GPU display width in pixels.
    int getWidth() const { return m_framebufferWidth; }

    // Return the emulated GPU display height in pixels.
    int getHeight() const { return m_framebufferHeight; }

    // Return the list of configs available from this display.
    // #if 0 //XXX :fix
    const FbConfigList* getConfigs() const { return m_configs; }
	// #else
	// FbConfigList* getConfigs() const { return m_configs; }
	// #endif
    // Set a callback that will be called each time the emulated GPU content
    // is updated. This can be relatively slow with host-based GPU emulation,
    // so only do this when you need to.
    void setPostCallback(emugl::Renderer::OnPostCallback onPost, void* onPostContext);

    // Retrieve the GL strings of the underlying EGL/GLES implementation.
    // On return, |*vendor|, |*renderer| and |*version| will point to strings
    // that are owned by the instance (and must not be freed by the caller).
    void getGLStrings(const char** vendor,
                      const char** renderer,
                      const char** version) const {
        *vendor = m_glVendor;
        *renderer = m_glRenderer;
        *version = m_glVersion;
    }

    // Create a new RenderContext instance for this display instance.
    // |p_config| is the index of one of the configs returned by getConfigs().
    // |p_share| is either EGL_NO_CONTEXT or the handle of a shared context.
    // |version| specifies the GLES version as a GLESApi enum.
    // Return a new handle value, which will be 0 in case of error.
    HandleType createRenderContext(int p_config, HandleType p_share, 
        GLESApi version = GLESApi_CM);

    // Create a new WindowSurface instance from this display instance.
    // |p_config| is the index of one of the configs returned by getConfigs().
    // |p_width| and |p_height| are the window dimensions in pixels.
    // Return a new handle value, or 0 in case of error.
    HandleType createWindowSurface(int p_config, int p_width, int p_height);

    // Create a new ColorBuffer instance from this display instance.
    // |p_width| and |p_height| are its dimensions in pixels.
    // |p_internalFormat| is the OpenGL format of this color buffer.
    // |p_frameworkFormat| describes the Android frameework format of this
    // color buffer, if differing from |p_internalFormat|.
    // See ColorBuffer::create() for
    // list of valid values. Note that ColorBuffer instances are reference-
    // counted. Use openColorBuffer / closeColorBuffer to operate on the
    // internal count.
    HandleType createColorBuffer(
        int p_width, int p_height, GLenum p_internalFormat,
        FrameworkFormat p_frameworkFormat);

    // Call this function when a render thread terminates to destroy all
    // the remaining contexts it created. Necessary to avoid leaking host
    // contexts when a guest application crashes, for example.
    void drainRenderContext();

    // Call this function when a render thread terminates to destroy all
    // remaining window surfqce it created. Necessary to avoid leaking
    // host buffers when a guest application crashes, for example.
    void drainWindowSurface();

    // Destroy a given RenderContext instance. |p_context| is its handle
    // value as returned by createRenderContext().
    void DestroyRenderContext(HandleType p_context);

    // Destroy a given WindowSurface instance. |p_surcace| is its handle
    // value as returned by createWindowSurface().
    void DestroyWindowSurface(HandleType p_surface);
    void DestroyWindowSurfaceLocked(HandleType p_surface);

    // Increment the reference count associated with a given ColorBuffer
    // instance. |p_colorbuffer| is its handle value as returned by
    // createColorBuffer().
    int openColorBuffer(HandleType p_colorbuffer);

    // Decrement the reference count associated with a given ColorBuffer
    // instance. |p_colorbuffer| is its handle value as returned by
    // createColorBuffer(). Note that if the reference count reaches 0,
    // the instance is destroyed automatically.
    void closeColorBuffer(HandleType p_colorbuffer);
    bool closeColorBufferLocked(HandleType p_colorbuffer);

    void cleanupProcGLObjects(uint64_t puid);
    // Equivalent for eglMakeCurrent() for the current display.
    // |p_context|, |p_drawSurface| and |p_readSurface| are the handle values
    // of the context, the draw surface and the read surface, respectively.
    // Returns true on success, false on failure.
    // Note: if all handle values are 0, this is an unbind operation.
    bool  bindContext(HandleType p_context,
                      HandleType p_drawSurface,
                      HandleType p_readSurface);

    // Return a render context pointer from its handle
    RenderContextPtr getContext_locked(HandleType p_context);

    // Return a color buffer pointer from its handle
    ColorBufferPtr getColorBuffer_locked(HandleType p_colorBuffer);

    // Return a color buffer pointer from its handle
    WindowSurfacePtr getWindowSurface_locked(HandleType p_windowsurface);

    // Attach a ColorBuffer to a WindowSurface instance.
    // See the documentation for WindowSurface::setColorBuffer().
    // |p_surface| is the target WindowSurface's handle value.
    // |p_colorbuffer| is the ColorBuffer handle value.
    // Returns true on success, false otherwise.
    bool  setWindowSurfaceColorBuffer(
            HandleType p_surface, HandleType p_colorbuffer);

    // Copy the content of a WindowSurface's Pbuffer to its attached
    // ColorBuffer. See the documentation for WindowSurface::flushColorBuffer()
    // |p_surface| is the target WindowSurface's handle value.
    // Returns true on success, false on failure.
    bool  flushWindowSurfaceColorBuffer(HandleType p_surface);

    // Bind the current context's EGL_TEXTURE_2D texture to a ColorBuffer
    // instance's EGLImage. This is intended to implement
    // glEGLImageTargetTexture2DOES() for all GLES versions.
    // |p_colorbuffer| is the ColorBuffer's handle value.
    // Returns true on success, false on failure.
    bool  bindColorBufferToTexture(HandleType p_colorbuffer);

    // Bind the current context's EGL_RENDERBUFFER_OES render buffer to this
    // ColorBuffer's EGLImage. This is intended to implement
    // glEGLImageTargetRenderbufferStorageOES() for all GLES versions.
    // |p_colorbuffer| is the ColorBuffer's handle value.
    // Returns true on success, false on failure.
    bool  bindColorBufferToRenderbuffer(HandleType p_colorbuffer);

    // Read the content of a given ColorBuffer into client memory.
    // |p_colorbuffer| is the ColorBuffer's handle value. Similar
    // to glReadPixels(), this can be a slow operation.
    // |x|, |y|, |width| and |height| are the position and dimensions of
    // a rectangle whose pixel values will be transfered to the host.
    // |format| indicates the format of the pixel data, e.g. GL_RGB or GL_RGBA.
    // |type| is the type of pixel data, e.g. GL_UNSIGNED_BYTE.
    // |pixels| is the address of a caller-provided buffer that will be filled
    // with the pixel data.
    void  readColorBuffer(HandleType p_colorbuffer,
                           int x, int y, int width, int height,
                           GLenum format, GLenum type, void *pixels);

    // Update the content of a given ColorBuffer from client data.
    // |p_colorbuffer| is the ColorBuffer's handle value. Similar
    // to glReadPixels(), this can be a slow operation.
    // |x|, |y|, |width| and |height| are the position and dimensions of
    // a rectangle whose pixel values will be transfered to the GPU
    // |format| indicates the format of the OpenGL buffer, e.g. GL_RGB or GL_RGBA.
    // |frameworkFormat| indicates the format of the pixel data; if
    // FRAMEWORK_FORMAT_GL_COMPATIBLE, |format| (OpenGL format) is used.
    // Otherwise, explicit conversion to |format| is needed.
    // |type| is the type of pixel data, e.g. GL_UNSIGNED_BYTE.
    // |pixels| is the address of a buffer containing the new pixel data.
    // Returns true on success, false otherwise.
    bool updateColorBuffer(HandleType p_colorbuffer,
                           int x, int y, int width, int height,
                           GLenum format, GLenum type, void *pixels);

    // Display the content of a given ColorBuffer into the framebuffer's
    // sub-window. |p_colorbuffer| is a handle value.
    // |needLockAndBind| is used to indicate whether the operation requires
    // acquiring/releasing the FrameBuffer instance's lock and binding the
    // contexts. It should be |false| only when called internally.
    bool post(HandleType p_colorbuffer, bool needLockAndBind = true);

    // Re-post the last ColorBuffer that was displayed through post().
    // This is useful if you detect that the sub-window content needs to
    // be re-displayed for any reason.
    bool repost();

    // Return the host EGLDisplay used by this instance.
    EGLDisplay getDisplay() const { return m_eglDisplay; }

    // Change the rotation of the displayed GPU sub-window.
    void setDisplayRotation(float zRot) {
        if (zRot != m_zRot) {
            m_zRot = zRot;
            repost();
        }
    }

    // Changes what coordinate of this framebuffer will be displayed at the
    // corner of the GPU sub-window. Specifically, |px| and |py| = 0 means
    // align the bottom-left of the framebuffer with the bottom-left of the
    // sub-window, and |px| and |py| = 1 means align the top right of the
    // framebuffer with the top right of the sub-window. Intermediate values
    // interpolate between these states.
    void setDisplayTranslation(float px, float py) {
        // Sanity check the values to ensure they are between 0 and 1
        const float x = px > 1.f ? 1.f : (px < 0.f ? 0.f : px);
        const float y = py > 1.f ? 1.f : (py < 0.f ? 0.f : py);
        if (x != m_px || y != m_py) {
            m_px = x;
            m_py = y;
            printf("setDisplayTranslation\n");
            repost();
        }
    }

    // Return a TextureDraw instance that can be used with this surfaces
    // and windows created by this instance.
    TextureDraw* getTextureDraw() const { return m_textureDraw; }

    // Create an eglImage and return its handle.  Reference:
    // https://www.khronos.org/registry/egl/extensions/KHR/EGL_KHR_image_base.txt
    HandleType createClientImage(HandleType context, EGLenum target, GLuint buffer);
    // Call the implementation of eglDestroyImageKHR, return if succeeds or
    // not. Reference:
    // https://www.khronos.org/registry/egl/extensions/KHR/EGL_KHR_image_base.txt
    EGLBoolean destroyClientImage(HandleType image);

    // Used internally.
    bool bind_locked();
    bool unbind_locked();

    void lockContextStructureRead() { m_contextStructureLock.lockRead(); }
    void unlockContextStructureRead() { m_contextStructureLock.unlockRead(); }

    // For use with sync threads and otherwise, any time we need a GL context
    // not specifically for drawing, but to obtain certain things about
    // GL state.
    // It can be unsafe / leaky to change the structure of contexts
    // outside the facilities the FrameBuffer class provides.
    void createTrivialContext(HandleType shared,
                              HandleType* contextOut,
                              HandleType* surfOut);

    void setShuttingDown() { m_shuttingDown = true; }

    ~FrameBuffer();
#if 0
    void onSave(android::base::Stream* stream);
    bool onLoad(android::base::Stream* stream);
#endif
    // lock and unlock handles (RenderContext, ColorBuffer, WindowSurface)
    void lock();
    void unlock();
private:
    FrameBuffer(int p_width, int p_height, bool useSubWindow);
    HandleType genHandle();

    bool bindSubwin_locked();
    bool removeSubWindow_locked();
    void cleanupProcGLObjects_locked(uint64_t puid);

private:
    static FrameBuffer *s_theFrameBuffer;
    static HandleType s_nextHandle;
    int m_x = 0;
    int m_y = 0;
    int m_framebufferWidth = 0;
    int m_framebufferHeight = 0;
    int m_windowWidth = 0;
    int m_windowHeight = 0;
    float m_dpr = 0;

    bool m_useSubWindow = false;
    bool m_eglContextInitialized = false;

    bool m_fpsStats = false;
    int m_statsNumFrames = 0;
    long long m_statsStartTime = 0;

    emugl::Mutex m_lock;
    emugl::ReadWriteMutex m_contextStructureLock;
    FbConfigList* m_configs = nullptr;
    //FBNativeWindowType m_nativeWindow = 0;
    FrameBufferCaps m_caps = {};
    EGLDisplay m_eglDisplay = EGL_NO_DISPLAY;
    RenderContextMap m_contexts;
    WindowSurfaceMap m_windows;
    ColorBufferMap m_colorbuffers;
    ColorBuffer::Helper* m_colorBufferHelper = nullptr;

    EGLSurface m_eglSurface = EGL_NO_SURFACE;
    EGLContext m_eglContext = EGL_NO_CONTEXT;
    EGLSurface m_pbufSurface = EGL_NO_SURFACE;
    EGLContext m_pbufContext = EGL_NO_CONTEXT;

    EGLContext m_prevContext = EGL_NO_CONTEXT;
    EGLSurface m_prevReadSurf = EGL_NO_SURFACE;
    EGLSurface m_prevDrawSurf = EGL_NO_SURFACE;
    EGLNativeWindowType m_subWin = {};
    TextureDraw* m_textureDraw = nullptr;
    EGLConfig  m_eglConfig = nullptr;
    HandleType m_lastPostedColorBuffer = 0;
    float      m_zRot = 0;
    float      m_px = 0;
    float      m_py = 0;

    emugl::Renderer::OnPostCallback m_onPost = nullptr;
    void* m_onPostContext = nullptr;
    unsigned char* m_fbImage = nullptr;

    const char* m_glVendor = nullptr;
    const char* m_glRenderer = nullptr;
    const char* m_glVersion = nullptr;

    // The host associates color buffers with guest processes for memory
    // cleanup. Guest processes are identified with a host generated unique ID.
    ProcOwnedWindowSurfaces m_procOwnedWindowSurfaces;
    ProcOwnedColorBuffers m_procOwnedColorBuffers;
    ProcOwnedEGLImages m_procOwnedEGLImages;
    ProcOwnedRenderContexts m_procOwnedRenderContext;

    // Flag set when emulator is shutting down.
    bool m_shuttingDown = false;
};
#endif
