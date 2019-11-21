/* Copyright (C) 2011 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

#include "android/opengles.h"
#include "android/opengl-snapshot.h"

#include "android/crashreport/crash-handler.h"
#include "android/emulation/GoldfishDma.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/globals.h"
#include "android/opengl/emugl_config.h"
#include "android/opengl/logger.h"
#include "android/utils/debug.h"
#include "android/utils/path.h"
#include "android/utils/bufprint.h"
#include "android/utils/dll.h"
#include "config-host.h"

#include "OpenglRender/render_api_functions.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define D(...)  VERBOSE_PRINT(init,__VA_ARGS__)
#define DD(...) VERBOSE_PRINT(gles,__VA_ARGS__)

/* Name of the GLES rendering library we're going to use */
#if UINTPTR_MAX == UINT32_MAX
#define RENDERER_LIB_NAME  "libOpenglRender"
#elif UINTPTR_MAX == UINT64_MAX
#define RENDERER_LIB_NAME  "lib64OpenglRender"
#else
#error Unknown UINTPTR_MAX
#endif

/* Declared in "android/globals.h" */
int  android_gles_fast_pipes = 1;

// Define the Render API function pointers.
#define FUNCTION_(ret, name, sig, params) \
        static ret (*name) sig = NULL;
LIST_RENDER_API_FUNCTIONS(FUNCTION_)
#undef FUNCTION_

// Define a function that initializes the function pointers by looking up
// the symbols from the shared library.
static int initOpenglesEmulationFuncs(ADynamicLibrary* rendererLib) {
    void*  symbol;
    char*  error;

#define FUNCTION_(ret, name, sig, params) \
    symbol = adynamicLibrary_findSymbol(rendererLib, #name, &error); \
    if (symbol != NULL) { \
        using type = ret(sig); \
        name = (type*)symbol; \
    } else { \
        derror("GLES emulation: Could not find required symbol (%s): %s", #name, error); \
        free(error); \
        return -1; \
    }
    LIST_RENDER_API_FUNCTIONS(FUNCTION_)
#undef FUNCTION_

    return 0;
}

static bool sRendererUsesSubWindow;
static emugl::RenderLibPtr sRenderLib = nullptr;
static emugl::RendererPtr sRenderer = nullptr;

int android_initOpenglesEmulation() {
    char* error = NULL;

    if (sRenderLib != NULL)
        return 0;

    D("Initializing hardware OpenGLES emulation support");

    ADynamicLibrary* const rendererSo =
            adynamicLibrary_open(RENDERER_LIB_NAME, &error);
    if (rendererSo == NULL) {
        derror("Could not load OpenGLES emulation library [%s]: %s",
               RENDERER_LIB_NAME, error);
        return -1;
    }

    /* Resolve the functions */
    if (initOpenglesEmulationFuncs(rendererSo) < 0) {
        derror("OpenGLES emulation library mismatch. Be sure to use the correct version!");
        goto BAD_EXIT;
    }

    sRenderLib = initLibrary();
    if (!sRenderLib) {
        derror("OpenGLES initialization failed!");
        goto BAD_EXIT;
    }

    sRendererUsesSubWindow = true;
    if (const char* env = getenv("ANDROID_GL_SOFTWARE_RENDERER")) {
        if (env[0] != '\0' && env[0] != '0') {
            sRendererUsesSubWindow = false;
        }
    }

    return 0;

BAD_EXIT:
    derror("OpenGLES emulation library could not be initialized!");
    adynamicLibrary_close(rendererSo);
    return -1;
}

int
android_startOpenglesRenderer(int width, int height, bool guestPhoneApi, int guestApiLevel,
                              int* glesMajorVersion_out,
                              int* glesMinorVersion_out)
{
    if (!sRenderLib) {
        D("Can't start OpenGLES renderer without support libraries");
        return -1;
    }

    if (sRenderer) {
        return 0;
    }

    android_init_opengl_logger();

    sRenderLib->setRenderer(emuglConfig_get_current_renderer());
    sRenderLib->setAvdInfo(guestPhoneApi, guestApiLevel);
    sRenderLib->setCrashReporter(&crashhandler_die_format);
    sRenderLib->setFeatureController(&android::featurecontrol::isEnabled);
    sRenderLib->setSyncDevice(goldfish_sync_create_timeline,
            goldfish_sync_create_fence,
            goldfish_sync_timeline_inc,
            goldfish_sync_destroy_timeline,
            goldfish_sync_register_trigger_wait,
            goldfish_sync_device_exists);

    emugl_logger_struct logfuncs;
    logfuncs.coarse = android_opengl_logger_write;
    logfuncs.fine = android_opengl_cxt_logger_write;
    sRenderLib->setLogger(logfuncs);
    emugl_dma_ops dma_ops;
    dma_ops.add_buffer = android_goldfish_dma_ops.add_buffer;
    dma_ops.remove_buffer = android_goldfish_dma_ops.remove_buffer;
    dma_ops.get_host_addr = android_goldfish_dma_ops.get_host_addr;
    dma_ops.invalidate_host_mappings = android_goldfish_dma_ops.invalidate_host_mappings;
    dma_ops.unlock = android_goldfish_dma_ops.unlock;
    sRenderLib->setDmaOps(dma_ops);

    sRenderer = sRenderLib->initRenderer(width, height, sRendererUsesSubWindow);
    if (!sRenderer) {
        D("Can't start OpenGLES renderer?");
        return -1;
    }

    // after initRenderer is a success, the maximum GLES API is calculated depending
    // on feature control and host GPU support. Set the obtained GLES version here.
    if (glesMajorVersion_out && glesMinorVersion_out)
        sRenderLib->getGlesVersion(glesMajorVersion_out, glesMinorVersion_out);
    return 0;
}

void
android_setPostCallback(OnPostFunc onPost, void* onPostContext)
{
    if (sRenderer) {
        sRenderer->setPostCallback(onPost, onPostContext);
    }
}

static char* strdupBaseString(const char* src) {
    const char* begin = strchr(src, '(');
    if (!begin) {
        return strdup(src);
    }

    const char* end = strrchr(begin + 1, ')');
    if (!end) {
        return strdup(src);
    }

    // src is of the form:
    // "foo (barzzzzzzzzzz)"
    //       ^            ^
    //       (b+1)        e
    //     = 5            18
    int len;
    begin += 1;
    len = end - begin;

    char* result;
    result = (char*)malloc(len + 1);
    memcpy(result, begin, len);
    result[len] = '\0';
    return result;
}

void android_getOpenglesHardwareStrings(char** vendor,
                                        char** renderer,
                                        char** version) {
    assert(vendor != NULL && renderer != NULL && version != NULL);
    assert(*vendor == NULL && *renderer == NULL && *version == NULL);
    if (!sRenderer) {
        D("Can't get OpenGL ES hardware strings when renderer not started");
        return;
    }

    const emugl::Renderer::HardwareStrings strings =
            sRenderer->getHardwareStrings();
    D("OpenGL Vendor=[%s]", strings.vendor.c_str());
    D("OpenGL Renderer=[%s]", strings.renderer.c_str());
    D("OpenGL Version=[%s]", strings.version.c_str());

    /* Special case for the default ES to GL translators: extract the strings
     * of the underlying OpenGL implementation. */
    if (strncmp(strings.vendor.c_str(), "Google", 6) == 0 &&
            strncmp(strings.renderer.c_str(), "Android Emulator OpenGL ES Translator", 37) == 0) {
        *vendor = strdupBaseString(strings.vendor.c_str());
        *renderer = strdupBaseString(strings.renderer.c_str());
        *version = strdupBaseString(strings.version.c_str());
    } else {
        *vendor = strdup(strings.vendor.c_str());
        *renderer = strdup(strings.renderer.c_str());
        *version = strdup(strings.version.c_str());
    }
}

void android_getOpenglesVersion(int* maj, int* min) {
    sRenderLib->getGlesVersion(maj, min);
    fprintf(stderr, "%s: maj min %d %d\n", __func__, *maj, *min);
}

void
android_stopOpenglesRenderer(void)
{
    if (sRenderer) {
        sRenderer->stop();
        sRenderer.reset();
        android_stop_opengl_logger();
    }
}

int
android_showOpenglesWindow(void* window, int wx, int wy, int ww, int wh,
                           int fbw, int fbh, float dpr, float rotation)
{
    if (!sRenderer) {
        return -1;
    }
    FBNativeWindowType win = (FBNativeWindowType)(uintptr_t)window;
    bool success = sRenderer->showOpenGLSubwindow(
            win, wx, wy, ww, wh, fbw, fbh, dpr, rotation);
    return success ? 0 : -1;
}

void
android_setOpenglesTranslation(float px, float py)
{
    if (sRenderer) {
        sRenderer->setOpenGLDisplayTranslation(px, py);
    }
}

int
android_hideOpenglesWindow(void)
{
    if (!sRenderer) {
        return -1;
    }
    bool success = sRenderer->destroyOpenGLSubwindow();
    return success ? 0 : -1;
}

void
android_redrawOpenglesWindow(void)
{
    if (sRenderer) {
        sRenderer->repaintOpenGLDisplay();
    }
}

const emugl::RendererPtr& android_getOpenglesRenderer()
{
    return sRenderer;
}

void android_cleanupProcGLObjects(uint64_t puid) {
    if (sRenderer) {
        sRenderer->cleanupProcGLObjects(puid);
    }
}
// add
int android_StartOpenglServer(char* addr, size_t addrLen)
{
	return StartOpenglServer(addr, addrLen);
}
