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
#include "OpenGLESDispatch/GLESv1Dispatch.h"
#include "OpenGLESDispatch/GLESv2Dispatch.h"

// #include "android/utils/debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#include "emugl/common/shared_library.h"
#include "emugl/common/osDynLibrary.h"

#define DEBUG 0

#if DEBUG
#define DPRINT(...) do { \
    if (!VERBOSE_CHECK(gles1emu)) VERBOSE_ENABLE(gles1emu); \
    VERBOSE_PRINT(gles1emu, __VA_ARGS__); \
} while (0)
#else
#define DPRINT(...)
#endif

static osUtils::dynLibrary *s_gles1_lib = NULL;
static osUtils::dynLibrary *s_underlying_gles2_lib = NULL;

// An unimplemented function which prints out an error message.
// To make it consistent with the guest, all GLES1 functions not supported by
// the guest driver should be redirected to this function.

static void gles1_unimplemented() {
    fprintf(stderr, "Called unimplemented GLESv1 API\n");
}

#define DEFAULT_GLES_CM_LIB EMUGL_LIBNAME("GLES_CM_translator")
#define DEFAULT_UNDERLYING_GLES_V2_LIB EMUGL_LIBNAME("GLES_V2_translator")

// This section of code (also in GLDispatch.cpp)
// initializes all GLESv1 functions to dummy ones;
// that is, in case we are using a library that doesn't
// have GLESv1, we will still have stubs available to
// signal that they are unsupported on the host.

#define RETURN_void return
#define RETURN_GLboolean return GL_FALSE
#define RETURN_GLint return 0
#define RETURN_GLuint return 0U
#define RETURN_GLenum return 0
#define RETURN_int return 0
#define RETURN_GLconstubyteptr return NULL
#define RETURN_voidptr return NULL

#define RETURN_(x)  RETURN_ ## x

#define DUMMY_MSG "Call to %s: host OpenGL driver does not support OpenGL ES v1. Skipping."

#ifdef _WIN32

#define DEFINE_DUMMY_FUNCTION(return_type, func_name, signature, args) \
static return_type  __stdcall gles1_dummy_##func_name signature { \
    fprintf(stderr, DUMMY_MSG, #func_name); \
    RETURN_(return_type); \
}

#define DEFINE_DUMMY_EXTENSION_FUNCTION(return_type, func_name, signature, args) \
static return_type __stdcall gles1_dummy_##func_name signature { \
    fprintf(stderr, DUMMY_MSG, #func_name); \
    RETURN_(return_type); \
}

#else

#define DEFINE_DUMMY_FUNCTION(return_type, func_name, signature, args) \
static return_type gles1_dummy_##func_name signature { \
    fprintf(stderr, DUMMY_MSG, #func_name); \
    RETURN_(return_type); \
}

#define DEFINE_DUMMY_EXTENSION_FUNCTION(return_type, func_name, signature, args) \
static return_type gles1_dummy_##func_name signature { \
    fprintf(stderr, DUMMY_MSG, #func_name); \
    RETURN_(return_type); \
}

#endif

LIST_GLES1_FUNCTIONS(DEFINE_DUMMY_FUNCTION, DEFINE_DUMMY_EXTENSION_FUNCTION);
LIST_GLES12_TR_FUNCTIONS(DEFINE_DUMMY_FUNCTION);

//
// This function is called only once during initialiation before
// any thread has been created - hence it should NOT be thread safe.
//

//
// init dummy GLESv1 dispatch table
//
#define ASSIGN_DUMMY(return_type,function_name,signature,callargs) do { \
        dispatch_table-> function_name = gles1_dummy_##function_name; \
        } while(0);

bool gles1_dispatch_init(GLESv1Dispatch* dispatch_table) {
    dispatch_table->underlying_gles2_api = NULL;

    const char* libName = getenv("ANDROID_GLESv1_LIB");
    if (!libName) {
        // libName = "libGLESv1_CM.so"; // DEFAULT_GLES_CM_LIB;
        libName = "/system/vendor/lib/egl/rk3288/libGLES_mali.so.bak";
    }

    // If emugl_config has detected specifically a backend
    // that supports only GLESv2, set GLESv1 entry points
    // to the dummy functions.
    if (!strcmp(libName, "<gles2_only_backend>")) {

        LIST_GLES1_FUNCTIONS(ASSIGN_DUMMY,ASSIGN_DUMMY)

        DPRINT("assigning dummies because <gles2_only_backend>");
        return true;
    } else {

        char error[256];
        s_gles1_lib = osUtils::dynLibrary::open(libName, error, sizeof(error));
        if (!s_gles1_lib) {
            fprintf(stderr, "%s: Could not load %s [%s]\n", __FUNCTION__,
                    libName, error);
            return false;
        }

        //
        // init the GLES dispatch table
        //
#define LOOKUP_SYMBOL(return_type,function_name,signature,callargs) do { \
        dispatch_table-> function_name = reinterpret_cast< function_name ## _t >( \
                s_gles1_lib->findSymbol(#function_name)); \
        } while(0);

        LIST_GLES1_FUNCTIONS(LOOKUP_SYMBOL,LOOKUP_SYMBOL)

        DPRINT("successful");

        LIST_GLES12_TR_FUNCTIONS(ASSIGN_DUMMY);

        // If we are using the translator,
        // import the gles1->2 translator dll
        if (strstr(libName, "GLES12Translator")) {

            DPRINT("trying to assign gles12-specific functions");
            LIST_GLES12_TR_FUNCTIONS(LOOKUP_SYMBOL);
            DPRINT("hopefully, successfully assigned "
                   "12tr-specific functions...");

            DPRINT("Now creating the underlying api");
            UnderlyingApis* gles2api =
                (UnderlyingApis*)dispatch_table->create_underlying_api();
            dispatch_table->underlying_gles2_api = gles2api;

            DPRINT("api ptr:%p", dispatch_table->underlying_gles2_api);

#define SET_UNDERLYING_GLES2_FUNC(rett, function_name, sig, callargs) do { \
    dispatch_table->underlying_gles2_api->angle-> function_name = \
        reinterpret_cast< function_name ## _t >( \
                s_underlying_gles2_lib->findSymbol(#function_name)); \
} while(0);

            DPRINT("trying to initialize GLESv1->2 translation");
            const char* underlying_gles2_lib_name =
                getenv("ANDROID_GLESv2_LIB");

            if (!underlying_gles2_lib_name) {
                underlying_gles2_lib_name = "/system/vendor/lib/egl/libGLES_mali.so.bak"; // DEFAULT_UNDERLYING_GLES_V2_LIB;
            }
            s_underlying_gles2_lib =
                osUtils::dynLibrary::open(underlying_gles2_lib_name,
                                           error, sizeof(error));
            if (!s_underlying_gles2_lib) {
                DPRINT("Could not load underlying gles2 lib %s [%s]",
                        libName, error);
                return false;
            }
            DPRINT("done trying to get gles2 lib");

            LIST_GLES2_FUNCTIONS(SET_UNDERLYING_GLES2_FUNC,
                                 SET_UNDERLYING_GLES2_FUNC);
       }
       return true;
    }
}

//
// This function is called only during initialization of the decoder before
// any thread has been created - hence it should NOT be thread safe.
//
void *gles1_dispatch_get_proc_func(const char *name, void *userData)
{
    void* func = NULL;
    if (s_gles1_lib) {
        func = (void *)s_gles1_lib->findSymbol(name);
    }
    // To make it consistent with the guest, redirect any unsupported functions
    // to gles1_unimplemented.
    if (!func) {
        func = (void *)gles1_unimplemented;
    }
    return func;
}
