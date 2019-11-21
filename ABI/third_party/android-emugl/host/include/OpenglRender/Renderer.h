// Copyright (C) 2016 The Android Open Source Project
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
#pragma once

// #include "OpenglRender/RenderChannel.h"
#include "OpenglRender/render_api_platform_types.h"
//#include "android/base/files/Stream.h"

#include <functional>
#include <memory>
#include <string>

namespace emugl {

// Renderer - an object that manages a single OpenGL window used for drawing
// and is able to create individual render channels for that window.
//
class Renderer {
public:
    // createRenderChannel - create a separate channel for the rendering data.
    // This call instantiates a new object that waits for the serialized data
    // from the guest, deserializes it, executes the passed GL commands and
    // returns the results back.
    // |loadStream| - if not NULL, RenderChannel uses it to load saved state
    //   asynchronously on its own thread. |loadStream| can be used right after
    //   the call as all the required data is copied here synchronously.
    //virtual RenderChannelPtr createRenderChannel(
    //        android::base::Stream* loadStream = nullptr) = 0;

    // getHardwareStrings - describe the GPU hardware and driver.
    // The underlying GL's vendor/renderer/version strings are returned to the
    // caller.
    struct HardwareStrings {
        std::string vendor;
        std::string renderer;
        std::string version;
    };
    virtual HardwareStrings getHardwareStrings() = 0;

    // A per-frame callback can be registered with setPostCallback(); to remove
    // it pass an empty callback. While a callback is registered, the renderer
    // will call it just before each new frame is displayed, providing a copy of
    // the framebuffer contents.
    //
    // The callback will be called from one of the renderer's threads, so will
    // probably need synchronization on any data structures it modifies. The
    // pixels buffer may be overwritten as soon as the callback returns; it
    // doesn't need to be synchronized, but if the callback needs the pixels
    // afterwards it must copy them.
    //
    // The pixels buffer is intentionally not const: the callback may modify the
    // data without copying to another buffer if it wants, e.g. in-place RGBA to
    // RGB conversion, or in-place y-inversion.
    //
    // Parameters are:
    //   width, height  Dimensions of the image, in pixels. Rows are tightly
    //                  packed; there is no inter-row padding.
    //   ydir           Indicates row order: 1 means top-to-bottom order, -1
    //                  means bottom-to-top order.
    //   format, type   Format and type GL enums, as used in glTexImage2D() or
    //                  glReadPixels(), describing the pixel format.
    //   pixels         The framebuffer image.
    //
    // In the first implementation, ydir is always -1 (bottom to top), format
    // and type are always GL_RGBA and GL_UNSIGNED_BYTE, and the width and
    // height will always be the same as the ones used to create the renderer.
    using OnPostCallback = void (*)(void* context,
                                    int width,
                                    int height,
                                    int ydir,
                                    int format,
                                    int type,
                                    unsigned char* pixels);
    virtual void setPostCallback(OnPostCallback onPost, void* context) = 0;

    // showOpenGLSubwindow -
    //     Create or modify a native subwindow which is a child of 'window'
    //     to be used for framebuffer display. If a subwindow already exists,
    //     its properties will be updated to match the given parameters.
    //     wx,wy is the top left corner of the rendering subwindow.
    //     ww,wh are the dimensions of the rendering subwindow.
    //     fbw,fbh are the dimensions of the underlying guest framebuffer.
    //     dpr is the device pixel ratio, which is needed for higher density
    //     displays like retina.
    //     zRot is the rotation to apply on the framebuffer display image.
    //
    //     Return true on success, false on failure, which can happen when using
    //     a software-only renderer like OSMesa. In this case, the client should
    //     call setPostCallback to get the content of each new frame when it is
    //     posted, and will be responsible for displaying it.
    virtual bool showOpenGLSubwindow(FBNativeWindowType window,
                                     int wx,
                                     int wy,
                                     int ww,
                                     int wh,
                                     int fbw,
                                     int fbh,
                                     float dpr,
                                     float zRot) = 0;

    // destroyOpenGLSubwindow -
    //   destroys the created native subwindow. Once destroyed,
    //   Framebuffer content will not be visible until a new
    //   subwindow will be created.
    virtual bool destroyOpenGLSubwindow() = 0;

    // setOpenGLDisplayRotation -
    //    set the framebuffer display image rotation in units
    //    of degrees around the z axis
    virtual void setOpenGLDisplayRotation(float zRot) = 0;

    // setOpenGLDisplayTranslation
    //    change what coordinate of the guest framebuffer will be displayed at
    //    the corner of the GPU sub-window. Specifically, |px| and |py| = 0
    //    means
    //    align the bottom-left of the framebuffer with the bottom-left of the
    //    sub-window, and |px| and |py| = 1 means align the top right of the
    //    framebuffer with the top right of the sub-window. Intermediate values
    //    interpolate between these states.
    virtual void setOpenGLDisplayTranslation(float px, float py) = 0;

    // repaintOpenGLDisplay -
    //    causes the OpenGL subwindow to get repainted with the
    //    latest framebuffer content.
    virtual void repaintOpenGLDisplay() = 0;

    // cleanupProcGLObjects -
    //    clean up all per-process resources when guest process exits (or is
    // killed). Such resources include color buffer handles and EglImage handles.
    virtual void cleanupProcGLObjects(uint64_t puid) = 0;

    // Stops all channels and render threads. The renderer cannot be used after
    // stopped.
    virtual void stop() = 0;
#if 0
    // Pauses all channels to prepare for snapshot saving.
    virtual void pauseAllPreSave() = 0;

    // Resumes all channels after snapshot saving or loading.
    virtual void resumeAll() = 0;

    virtual void save(android::base::Stream* stream) = 0;
    virtual bool load(android::base::Stream* stream) = 0;
#endif
protected:
    ~Renderer() = default;
};

// using RendererPtr = std::shared_ptr<Renderer>;// FIXME

}  // namespace emugl