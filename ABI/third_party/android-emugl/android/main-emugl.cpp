// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/main-emugl.h"

#include "android/avd/util.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/opengl/gpuinfo.h"
#include "android/utils/debug.h"
#include "android/utils/string.h"

#include <stdlib.h>
#include <string.h>

using android::base::ScopedCPtr;

bool androidEmuglConfigInit(EmuglConfig* config,
                            const char* avdName,
                            const char* avdArch,
                            int apiLevel,
                            bool hasGoogleApis,
                            const char* gpuOption,
                            int wantedBitness,
                            bool noWindow,
                            enum WinsysPreferredGlesBackend uiPreferredBackend) {
    bool gpuEnabled = false;
    ScopedCPtr<char> gpuMode;

    if (avdName) {
        gpuMode.reset(path_getAvdGpuMode(avdName));
        gpuEnabled = (gpuMode.get() != nullptr);
    } else if (!gpuOption) {
        // In the case of a platform build, use the 'auto' mode by default.
        gpuMode.reset(::strdup("auto"));
        gpuEnabled = true;
    }

    // Detect if this is google API's

    bool hasGuestRenderer = (!strcmp(avdArch, "x86") ||
                             !strcmp(avdArch, "x86_64")) &&
                             (apiLevel >= 23) &&
                             hasGoogleApis;

    bool blacklisted = false;
    bool onBlacklist = false;

    const char* gpuChoice = gpuOption ? gpuOption : gpuMode.get();

    // If the user has specified a renderer
    // that is neither "auto" nor "host",
    // don't check the blacklist.
    // Only check the blacklist for 'auto' or 'host' mode.
    if (gpuChoice && (!strcmp(gpuChoice, "auto") ||
            !strcmp(gpuChoice, "host"))) {
         onBlacklist = isHostGpuBlacklisted();
    }

    if (avdName) {
        // This is for testing purposes only.
        ScopedCPtr<const char> testGpuBlacklist(
                path_getAvdGpuBlacklisted(avdName));
        if (testGpuBlacklist.get()) {
            onBlacklist = !strcmp(testGpuBlacklist.get(), "yes");
        }
    }

    if (gpuChoice && !strcmp(gpuChoice, "auto")) {
        if (onBlacklist) {
            dwarning("Your GPU drivers may have a bug. "
                     "Switching to software rendering.");
        }
        blacklisted = onBlacklist;
        setGpuBlacklistStatus(blacklisted);
    } else if (onBlacklist && gpuChoice &&
            (!strcmp(gpuChoice, "host") || !strcmp(gpuChoice, "on"))) {
        dwarning("Your GPU drivers may have a bug. "
                 "If you experience graphical issues, "
                 "please consider switching to software rendering.");
    }

    bool result = emuglConfig_init(
            config, gpuEnabled, gpuMode.get(), gpuOption, wantedBitness,
            noWindow, blacklisted, hasGuestRenderer, uiPreferredBackend);

    return result;
}
