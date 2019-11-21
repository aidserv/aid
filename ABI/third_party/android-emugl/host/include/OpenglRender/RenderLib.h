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

#include <memory>

//#include "android/base/files/Stream.h"
#include "android/opengl/emugl_config.h"
#include "OpenglRender/Renderer.h"
#include "OpenglRender/render_api_types.h"

// add 
// #include "shared/emugl/common/sync_device.h"
#if 0
typedef enum SelectedRenderer {
    SELECTED_RENDERER_UNKNOWN = 0,
    SELECTED_RENDERER_HOST = 1,
    SELECTED_RENDERER_OFF = 2,
    SELECTED_RENDERER_GUEST = 3,
    SELECTED_RENDERER_MESA = 4,
    SELECTED_RENDERER_SWIFTSHADER = 5,
    SELECTED_RENDERER_ANGLE = 6, // ANGLE D3D11 with D3D9 fallback
    SELECTED_RENDERER_ANGLE9 = 7, // ANGLE forced to D3D9
    SELECTED_RENDERER_ERROR = 255,
} SelectedRenderer;
#endif
namespace emugl {

// RenderLib - root interface for the GPU emulation library
//  Use it to set the library-wide parameters (logging, crash reporting) and
//  create indivudual renderers that take care of drawing windows.
class RenderLib {
public:
    virtual ~RenderLib() = default;

    // Get the selected renderer
    virtual void setRenderer(SelectedRenderer renderer) = 0;
#if 0
    // Tell emugl the API version of the system image
    virtual void setAvdInfo(bool phone, int api) = 0;
    // Get the GLES major/minor version determined by libOpenglRender.
    virtual void getGlesVersion(int* maj, int* min) = 0;
    virtual void setLogger(emugl_logger_struct logger) = 0;
    virtual void setCrashReporter(emugl_crash_reporter_t reporter) = 0;
    virtual void setFeatureController(emugl_feature_is_enabled_t featureController) = 0;
    virtual void setSyncDevice(emugl_sync_create_timeline_t,
                               emugl_sync_create_fence_t,
                               emugl_sync_timeline_inc_t,
                               emugl_sync_destroy_timeline_t,
                               emugl_sync_register_trigger_wait_t,
                               emugl_sync_device_exists_t) = 0;

    // Sets the function use to read from the guest
    // physically contiguous DMA region at particular offsets.
    virtual void setDmaOps(emugl_dma_ops) = 0;
#endif
    // initRenderer - initialize the OpenGL renderer object.
    //
    // |width| and |height| are the framebuffer dimensions that will be reported
    // to the guest display driver.
    //
    // |useSubWindow| is true to indicate that renderer has to support an
    // OpenGL subwindow. If false, it only supports setPostCallback().
    // See Renderer.h for more info.
    //
    // There might be only one renderer.
    virtual Renderer* initRenderer(int width, int height,
                                     bool useSubWindow) = 0;
};

// using RenderLibPtr = std::unique_ptr<RenderLib>;// FIXME

}  // namespace emugl
