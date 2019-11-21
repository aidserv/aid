// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/opengl/emugl_config.h"
#include "android/opengl/gpuinfo.h"

#include "android/base/StringFormat.h"
#include "android/base/system/System.h"
#include "android/globals.h"
#include "android/opengl/EmuglBackendList.h"
#include "android/skin/winsys.h"

#include <string>

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEBUG 0

#if DEBUG
#define D(...)  printf(__VA_ARGS__)
#else
#define D(...)  ((void)0)
#endif

using android::base::RunOptions;
using android::base::StringFormat;
using android::base::System;
using android::opengl::EmuglBackendList;

static EmuglBackendList* sBackendList = NULL;

static void resetBackendList(int bitness) {
    delete sBackendList;
    sBackendList = new EmuglBackendList(
            System::get()->getLauncherDirectory().c_str(), bitness);
}

static bool stringVectorContains(const std::vector<std::string>& list,
                                 const char* value) {
    for (size_t n = 0; n < list.size(); ++n) {
        if (!strcmp(list[n].c_str(), value)) {
            return true;
        }
    }
    return false;
}

bool isHostGpuBlacklisted() {
    return async_query_host_gpu_blacklisted();
}

// Get a description of host GPU properties.
// Need to free after use.
emugl_host_gpu_prop_list emuglConfig_get_host_gpu_props() {
    const GpuInfoList& gpulist = globalGpuInfoList();
    emugl_host_gpu_prop_list res;
    res.num_gpus = gpulist.infos.size();
    res.props = new emugl_host_gpu_props[res.num_gpus];

    const std::vector<GpuInfo>& infos = gpulist.infos;
    for (int i = 0; i < res.num_gpus; i++) {
        res.props[i].make = strdup(infos[i].make.c_str());
        res.props[i].model = strdup(infos[i].model.c_str());
        res.props[i].device_id = strdup(infos[i].device_id.c_str());
        res.props[i].revision_id = strdup(infos[i].revision_id.c_str());
        res.props[i].version = strdup(infos[i].version.c_str());
        res.props[i].renderer = strdup(infos[i].renderer.c_str());
    }
    return res;
}

SelectedRenderer emuglConfig_get_renderer(const char* gpu_mode) {
    if (!gpu_mode) {
        return SELECTED_RENDERER_UNKNOWN;
    } else if (!strcmp(gpu_mode, "host") ||
        !strcmp(gpu_mode, "on")) {
        return SELECTED_RENDERER_HOST;
    } else if (!strcmp(gpu_mode, "off")) {
        return SELECTED_RENDERER_OFF;
    } else if (!strcmp(gpu_mode, "guest")) {
        return SELECTED_RENDERER_GUEST;
    } else if (!strcmp(gpu_mode, "mesa")) {
        return SELECTED_RENDERER_MESA;
    } else if (!strcmp(gpu_mode, "swiftshader")) {
        return SELECTED_RENDERER_SWIFTSHADER;
    } else if (!strcmp(gpu_mode, "angle")) {
        return SELECTED_RENDERER_ANGLE;
    } else if (!strcmp(gpu_mode, "angle9")) {
        return SELECTED_RENDERER_ANGLE9;
    } else if (!strcmp(gpu_mode, "error")) {
        return SELECTED_RENDERER_ERROR;
    } else {
        return SELECTED_RENDERER_UNKNOWN;
    }
}

static SelectedRenderer sCurrentRenderer =
    SELECTED_RENDERER_UNKNOWN;

SelectedRenderer emuglConfig_get_current_renderer() {
    return sCurrentRenderer;
}

void free_emugl_host_gpu_props(emugl_host_gpu_prop_list proplist) {
    for (int i = 0; i < proplist.num_gpus; i++) {
        free(proplist.props[i].make);
        free(proplist.props[i].model);
        free(proplist.props[i].device_id);
        free(proplist.props[i].revision_id);
        free(proplist.props[i].version);
        free(proplist.props[i].renderer);
    }
    delete [] proplist.props;
}

static void setCurrentRenderer(const char* gpuMode) {
    sCurrentRenderer = emuglConfig_get_renderer(gpuMode);
}

bool emuglConfig_init(EmuglConfig* config,
                      bool gpu_enabled,
                      const char* gpu_mode,
                      const char* gpu_option,
                      int bitness,
                      bool no_window,
                      bool blacklisted,
                      bool has_guest_renderer,
                      enum WinsysPreferredGlesBackend uiPreferredBackend) {
    D("%s: blacklisted=%d has_guest_renderer=%d\n",
      __FUNCTION__,
      blacklisted,
      has_guest_renderer);

    // zero all fields first.
    memset(config, 0, sizeof(*config));

    bool host_set_in_hwconfig = false;

    bool hasUiPreference = uiPreferredBackend != WINSYS_GLESBACKEND_PREFERENCE_AUTO;

    // The value of '-gpu <mode>' overrides both the hardware properties
    // and the UI setting, except if <mode> is 'auto'.
    if (gpu_option) {
        if (!strcmp(gpu_option, "on") || !strcmp(gpu_option, "enable")) {
            gpu_enabled = true;
            if (!gpu_mode || !strcmp(gpu_mode, "auto")) {
                gpu_mode = "host";
            }
        } else if (!strcmp(gpu_option, "off") ||
                   !strcmp(gpu_option, "disable") ||
                   !strcmp(gpu_option, "guest")) {
            gpu_mode = gpu_option;
            gpu_enabled = false;
        } else if (!strcmp(gpu_option, "auto")) {
            // Nothing to do, use gpu_mode set from
            // hardware properties instead.
        } else {
            gpu_enabled = true;
            gpu_mode = gpu_option;
        }
    } else {
        // Support "hw.gpu.mode=on" in config.ini
        if (gpu_enabled && gpu_mode && (
            !strcmp(gpu_mode, "on") ||
            !strcmp(gpu_mode, "enable") ||
            !strcmp(gpu_mode, "host"))) {
            gpu_enabled = true;
            gpu_mode = "host";
            host_set_in_hwconfig = true;
        }
    }

    if (gpu_mode &&
        (!strcmp(gpu_mode, "guest") ||
         !strcmp(gpu_mode, "off"))) {
        gpu_enabled = false;
    }

    if (!gpu_option && hasUiPreference) {
        gpu_enabled = true;
        gpu_mode = "auto";
    }

    if (!gpu_enabled) {
        config->enabled = false;
        snprintf(config->backend, sizeof(config->backend), "%s", gpu_mode);
        snprintf(config->status, sizeof(config->status),
                 "GPU emulation is disabled");
        return true;
    }

    if (!bitness) {
        bitness = System::get()->getProgramBitness();
    }

    config->bitness = bitness;
    resetBackendList(bitness);

    // Check that the GPU mode is a valid value. 'auto' means determine
    // the best mode depending on the environment. Its purpose is to
    // enable 'swiftshader' mode automatically when NX or Chrome Remote Desktop
    // is detected.
    if (!strcmp(gpu_mode, "auto") || host_set_in_hwconfig) {
        // The default will be 'host' unless:
        // 1. NX or Chrome Remote Desktop is detected, or |no_window| is true.
        // 2. The user's host GPU is on the blacklist.
        std::string sessionType;
        if (System::get()->isRemoteSession(&sessionType)) {
            D("%s: %s session detected\n", __FUNCTION__, sessionType.c_str());
            if (!sBackendList->contains("swiftshader")) {
                config->enabled = false;
                gpu_mode = "off";
                snprintf(config->backend, sizeof(config->backend), "%s", gpu_mode);
                snprintf(config->status, sizeof(config->status),
                        "GPU emulation is disabled under %s without Swiftshader",
                        sessionType.c_str());
                setCurrentRenderer(gpu_mode);
                return true;
            }
            D("%s: 'swiftshader' mode auto-selected\n", __FUNCTION__);
            gpu_mode = "swiftshader";
        }
        else if (!host_set_in_hwconfig && !no_window && !hasUiPreference &&
                   async_query_host_gpu_AngleWhitelisted()) {
                gpu_mode = "angle";
                D("%s use Angle for Intel GPU HD 3000\n", __FUNCTION__);
        }
        else if (no_window || (blacklisted && !hasUiPreference)) {
            if (stringVectorContains(sBackendList->names(), "swiftshader")) {
                D("%s: Headless (-no-window) mode (or blacklisted GPU driver)"
                  ", using Swiftshader backend\n",
                  __FUNCTION__);
                gpu_mode = "swiftshader";
            } else if (!has_guest_renderer) {
                D("%s: Headless (-no-window) mode (or blacklisted GPU driver)"
                  " without Swiftshader, forcing '-gpu off'\n",
                  __FUNCTION__);
                config->enabled = false;
                gpu_mode = "off";
                snprintf(config->backend, sizeof(config->backend), "%s", gpu_mode);
                snprintf(config->status, sizeof(config->status),
                        "GPU emulation is disabled (-no-window without Swiftshader)");
                setCurrentRenderer(gpu_mode);
                return true;
            } else {
                D("%s: Headless (-no-window) mode (or blacklisted GPU driver)"
                  ", using guest GPU backend\n",
                  __FUNCTION__);
                config->enabled = false;
                gpu_mode = "off";
                snprintf(config->backend, sizeof(config->backend), "%s", gpu_mode);
                snprintf(config->status, sizeof(config->status),
                        "GPU emulation is in the guest");
                gpu_mode = "guest";
                setCurrentRenderer(gpu_mode);
                return true;
            }
        } else {
            switch (uiPreferredBackend) {
                case WINSYS_GLESBACKEND_PREFERENCE_ANGLE:
                    gpu_mode = "angle";
                    break;
                case WINSYS_GLESBACKEND_PREFERENCE_ANGLE9:
                    gpu_mode = "angle9";
                    break;
                case WINSYS_GLESBACKEND_PREFERENCE_SWIFTSHADER:
                    gpu_mode = "swiftshader";
                    break;
                case WINSYS_GLESBACKEND_PREFERENCE_NATIVEGL:
                    gpu_mode = "host";
                    break;
                default:
                    gpu_mode = "host";
                    break;
            }
            D("%s: auto-selected %s based on conditions and UI preference %d\n",
              __func__, gpu_mode, uiPreferredBackend);
        }
    }

    // 'host' is a special value corresponding to the default translation
    // to desktop GL, 'guest' does not use host-side emulation,
    // anything else must be checked against existing host-side backends.
    if (strcmp(gpu_mode, "host") != 0 && strcmp(gpu_mode, "guest") != 0) {
        const std::vector<std::string>& backends = sBackendList->names();
        if (!stringVectorContains(backends, gpu_mode)) {
            std::string error = StringFormat(
                "Invalid GPU mode '%s', use one of: on off host guest", gpu_mode);
            for (size_t n = 0; n < backends.size(); ++n) {
                error += " ";
                error += backends[n];
            }
            config->enabled = false;
            gpu_mode = "error";
            snprintf(config->backend, sizeof(config->backend), "%s", gpu_mode);
            snprintf(config->status, sizeof(config->status), "%s",
                     error.c_str());
            setCurrentRenderer(gpu_mode);
            return false;
        }
    }

    if (strcmp(gpu_mode, "guest")) {
        config->enabled = true;
    }

    snprintf(config->backend, sizeof(config->backend), "%s", gpu_mode);
    snprintf(config->status, sizeof(config->status),
             "GPU emulation enabled using '%s' mode", gpu_mode);
    setCurrentRenderer(gpu_mode);
    return true;
}

void emuglConfig_setupEnv(const EmuglConfig* config) {
    System* system = System::get();

    if (!config->enabled) {
        // There is no real GPU emulation. As a special case, define
        // SDL_RENDER_DRIVER to 'software' to ensure that the
        // software SDL renderer is being used. This allows one
        // to run with '-gpu off' under NX and Chrome Remote Desktop
        // properly.
        system->envSet("SDL_RENDER_DRIVER", "software");
        return;
    }

    // $EXEC_DIR/<lib>/ is already added to the library search path by default,
    // since generic libraries are bundled there. We may need more though:
    resetBackendList(config->bitness);
    if (strcmp(config->backend, "host") != 0) {
        // If the backend is not 'host', we also need to add the
        // backend directory.
        std::string dir = sBackendList->getLibDirPath(config->backend);
        if (dir.size()) {
            D("Adding to the library search path: %s\n", dir.c_str());
            system->addLibrarySearchDir(dir);
        }
    }

    if (!strcmp(config->backend, "host")) {
        // Nothing more to do for the 'host' backend.
        return;
    }

    // For now, EmuGL selects its own translation libraries for
    // EGL/GLES libraries, unless the following environment
    // variables are defined:
    //    ANDROID_EGL_LIB
    //    ANDROID_GLESv1_LIB
    //    ANDROID_GLESv2_LIB
    //
    // If a backend provides one of these libraries, use it.
    std::string lib;
    if (sBackendList->getBackendLibPath(
            config->backend, EmuglBackendList::LIBRARY_EGL, &lib)) {
        system->envSet("ANDROID_EGL_LIB", lib);
    }
    if (sBackendList->getBackendLibPath(
            config->backend, EmuglBackendList::LIBRARY_GLESv1, &lib)) {
        system->envSet("ANDROID_GLESv1_LIB", lib);
    } else if (strcmp(config->backend, "mesa")) {
        fprintf(stderr, "OpenGL backend '%s' without OpenGL ES 1.x library detected. "
                        "Using GLESv2 only.\n",
                        config->backend);
        // A GLESv1 lib is optional---we can deal with a GLESv2 only
        // backend by using a GLESv1->GLESv2 emulation library.
        system->envSet("ANDROID_GLESv1_LIB", sBackendList->getGLES12TranslatorLibName().c_str());
    }

    if (sBackendList->getBackendLibPath(
            config->backend, EmuglBackendList::LIBRARY_GLESv2, &lib)) {
        system->envSet("ANDROID_GLESv2_LIB", lib);
    }

    if (!strcmp(config->backend, "mesa")) {
        fprintf(stderr, "WARNING: The Mesa software renderer is deprecated. "
                        "Use Swiftshader (-gpu swiftshader) for software rendering.\n");
        system->envSet("ANDROID_GL_LIB", "mesa");
        system->envSet("ANDROID_GL_SOFTWARE_RENDERER", "1");
    }
}
