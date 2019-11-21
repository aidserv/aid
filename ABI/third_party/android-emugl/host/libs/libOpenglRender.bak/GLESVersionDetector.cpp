/*
* Copyright (C) 2016 The Android Open Source Project
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

#include "GLESVersionDetector.h"

#include "DispatchTables.h"

#include "emugl/common/feature_control.h"
#include "emugl/common/misc.h"

#include <algorithm>

GLESDispatchMaxVersion calcMaxVersionFromDispatch() {
    // Detect the proper OpenGL ES version to advertise to the system.
    //
    // If the underlying glesv2 library also is on top of system OpenGL,
    // we assume existence of a function called "gl_dispatch_get_max_version"
    // which matches the return type of gles2_dispatch_get_max_version,
    // and allows us to scale back if the underlying host machine
    // doesn't actually suppport a particular GLES version.
    GLESDispatchMaxVersion res = GLES_DISPATCH_MAX_VERSION_2;

    if (emugl_feature_is_enabled(android::featurecontrol::GLESDynamicVersion)) {
        GLESDispatchMaxVersion underlying_gles2_lib_max =
            gles2_dispatch_get_max_version();
        if (s_gles2.gl_dispatch_get_max_version) {
            GLESDispatchMaxVersion underlying_gl_lib_max =
                (GLESDispatchMaxVersion)s_gles2.gl_dispatch_get_max_version();
            res = std::min(underlying_gl_lib_max, underlying_gles2_lib_max);
        } else {
            res = underlying_gles2_lib_max;
        }
    }

    // TODO: Remove restriction when core profile is working
#ifdef __APPLE__
    if (emugl::getRenderer() != SELECTED_RENDERER_SWIFTSHADER) {
        res = GLES_DISPATCH_MAX_VERSION_2;
    }
#endif

    // TODO: Modify when CTS compliant for OpenGL ES 3.1
    if (res >= GLES_DISPATCH_MAX_VERSION_3_1 &&
        emugl_feature_is_enabled(android::featurecontrol::PlayStoreImage)) {
        res = GLES_DISPATCH_MAX_VERSION_3_0;
    }

    int maj = 2; int min = 0;
    switch (res) {
        case GLES_DISPATCH_MAX_VERSION_2:
            maj = 2; min = 0; break;
        case GLES_DISPATCH_MAX_VERSION_3_0:
            maj = 3; min = 0; break;
        case GLES_DISPATCH_MAX_VERSION_3_1:
            maj = 3; min = 1; break;
        case GLES_DISPATCH_MAX_VERSION_3_2:
            maj = 3; min = 2; break;
        default:
            break;
    }

    emugl::setGlesVersion(maj, min);

    return res;
}
