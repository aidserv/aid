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
#include "OpenGLESDispatch/GLESv2Dispatch.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#include "emugl/common/shared_library.h"
#include "emugl/common/osDynLibrary.h"

static osUtils::dynLibrary *s_gles2_lib = NULL;

#define DEFAULT_GLES_V2_LIB EMUGL_LIBNAME("GLES_V2_translator")

// An unimplemented function which prints out an error message.
// To make it consistent with the guest, all GLES2 functions not supported by
// the driver should be redirected to this function.

void gles2_unimplemented() {
    fprintf(stderr, "Called unimplemented GLES API\n");
}

// Holds the level of GLES 3.x support after gles2_dispatch_init runs.
static GLESDispatchMaxVersion s_max_supported_gles_version = GLES_DISPATCH_MAX_VERSION_2;

//
// This function is called only once during initialiation before
// any thread has been created - hence it should NOT be thread safe.
//
bool gles2_dispatch_init(GLESv2Dispatch* dispatch_table)
{
    const char *libName = getenv("ANDROID_GLESv2_LIB");
    if (!libName) {
        libName = "/system/vendor/lib/egl/rk3288/libGLES_mali.so.bak"; // DEFAULT_GLES_V2_LIB;
    }

    char error[256];
    s_gles2_lib = osUtils::dynLibrary::open(libName, error, sizeof(error));
    if (!s_gles2_lib) {
        fprintf(stderr, "%s: Could not load %s [%s]\n", __FUNCTION__,
                libName, error);
        return false;
    }

    //
    // init the GLES dispatch table
    //
#define LOOKUP_SYMBOL(return_type,function_name,signature,callargs) \
    dispatch_table-> function_name = reinterpret_cast< function_name ## _t >( \
            s_gles2_lib->findSymbol(#function_name));

    LIST_GLES2_FUNCTIONS(LOOKUP_SYMBOL,LOOKUP_SYMBOL)

    // Now detect the maximum level of GLES 3.x support
    // advertised by the underlying GLESv2 lib.
    // Note that if the underlying GLESv2 lib makes assumptions
    // about what is supported on the system OpenGL a layer below,
    // the result from this check may not be accurate.
    bool gles30_supported = true;
    bool gles31_supported = false; // false
    bool gles32_supported = false;
    // For 3.0, we don't really need glInvalidate(Sub)Framebuffer.
#define DETECT_GLES30_SUPPORT(return_type, function_name, signature, callargs) do { \
    if (!dispatch_table->function_name && \
        strcmp(#function_name, "glInvalidateFramebuffer") && \
        strcmp(#function_name, "glInvalidateSubFramebuffer") ) { \
        gles30_supported = false; \
    } \
    } while(0); \

    LIST_GLES3_ONLY_FUNCTIONS(DETECT_GLES30_SUPPORT)

#define DETECT_GLES31_SUPPORT(return_type, function_name, signature, callargs) do { \
    if (!dispatch_table->function_name) { \
        gles31_supported = false; } \
    } while(0); \

    LIST_GLES31_ONLY_FUNCTIONS(DETECT_GLES31_SUPPORT)

    if (gles30_supported && gles31_supported && gles32_supported) {
        s_max_supported_gles_version = GLES_DISPATCH_MAX_VERSION_3_2;
    } else if (gles30_supported && gles31_supported) {
        s_max_supported_gles_version = GLES_DISPATCH_MAX_VERSION_3_1;
    } else if (gles30_supported) {
        s_max_supported_gles_version = GLES_DISPATCH_MAX_VERSION_3_0;
    } else {
        s_max_supported_gles_version = GLES_DISPATCH_MAX_VERSION_2;
    }
	printf("x12344 s_max_supported_gles_version=%d\n", s_max_supported_gles_version);
    return true;
}

//
// This function is called only during initialization before
// any thread has been created - hence it should NOT be thread safe.
//
void *gles2_dispatch_get_proc_func(const char *name, void *userData)
{
    void* func = NULL;
    if (s_gles2_lib) {
        func = (void *)s_gles2_lib->findSymbol(name);
    }
    // To make it consistent with the guest, redirect any unsupported functions
    // to gles2_unimplemented.
    if (!func) {
        func = (void *)gles2_unimplemented;
    }
    return func;
}

GLESDispatchMaxVersion gles2_dispatch_get_max_version() {
    return s_max_supported_gles_version;
}
