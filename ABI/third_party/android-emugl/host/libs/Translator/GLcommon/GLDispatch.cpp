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

#include "GLcommon/GLDispatch.h"
#include "GLcommon/GLLibrary.h"

#include "emugl/common/lazy_instance.h"
#include "emugl/common/shared_library.h"

#include "OpenglCodecCommon/ErrorLog.h"

#ifdef __linux__
#include <GL/glx.h>
#elif defined(WIN32)
#include <windows.h>
#endif

#include <stdio.h>
#include <string.h>

typedef GlLibrary::GlFunctionPointer GL_FUNC_PTR;

static GL_FUNC_PTR getGLFuncAddress(const char *funcName, GlLibrary* glLib) {
    return glLib->findSymbol(funcName);
}

#define LOAD_GL_FUNC(return_type, func_name, signature, args)  do { \
        if (!func_name) { \
            void* address = (void *)getGLFuncAddress(#func_name, glLib); \
            if (address) { \
                func_name = (__typeof__(func_name))(address); \
            } else { \
                func_name = (__typeof__(func_name))(dummy_##func_name); \
            } \
        } \
    } while (0);

#define LOAD_GLEXT_FUNC(return_type, func_name, signature, args) do { \
        if (!func_name) { \
            void* address = (void *)getGLFuncAddress(#func_name, glLib); \
            if (address) { \
                func_name = (__typeof__(func_name))(address); \
            } \
        } \
    } while (0);

// Define dummy functions, only for non-extensions.

#define RETURN_void return
#define RETURN_GLboolean return GL_FALSE
#define RETURN_GLint return 0
#define RETURN_GLuint return 0U
#define RETURN_GLenum return 0
#define RETURN_int return 0
#define RETURN_GLconstubyteptr return NULL

#define RETURN_(x)  RETURN_ ## x

#define DEFINE_DUMMY_FUNCTION(return_type, func_name, signature, args) \
static return_type dummy_##func_name signature { \
    RETURN_(return_type); \
}

#define DEFINE_DUMMY_EXTENSION_FUNCTION(return_type, func_name, signature, args) \
  // nothing here

LIST_GLES_FUNCTIONS(DEFINE_DUMMY_FUNCTION, DEFINE_DUMMY_EXTENSION_FUNCTION)

// Initializing static GLDispatch members*/

emugl::Mutex GLDispatch::s_lock;

#define GL_DISPATCH_DEFINE_POINTER(return_type, function_name, signature, args) \
    GL_APICALL return_type (GL_APIENTRY *GLDispatch::function_name) signature = NULL;

LIST_GLES_FUNCTIONS(GL_DISPATCH_DEFINE_POINTER, GL_DISPATCH_DEFINE_POINTER)

// Constructor.
GLDispatch::GLDispatch() : m_isLoaded(false) {}

bool GLDispatch::isInitialized() const {
    return m_isLoaded;
}

// Holds the level of GLES 3.x support after dispatchFuncs runs.
static GLDispatchMaxGLESVersion s_max_supported_gles_version = GL_DISPATCH_MAX_GLES_VERSION_2;
static bool s_got_gles_support_level = false;

void GLDispatch::dispatchFuncs(GLESVersion version, GlLibrary* glLib) {
    emugl::Mutex::AutoLock mutex(s_lock);
    if(m_isLoaded)
        return;

    /* Loading OpenGL functions which are needed for implementing BOTH GLES 1.1 & GLES 2.0*/
    LIST_GLES_COMMON_FUNCTIONS(LOAD_GL_FUNC)
    LIST_GLES_EXTENSIONS_FUNCTIONS(LOAD_GLEXT_FUNC)

    /* Loading OpenGL functions which are needed ONLY for implementing GLES 1.1*/
    if(version == GLES_1_1){
        LIST_GLES1_ONLY_FUNCTIONS(LOAD_GL_FUNC)
        LIST_GLES1_EXTENSIONS_FUNCTIONS(LOAD_GLEXT_FUNC)
        LIST_GLES2_ONLY_FUNCTIONS(LOAD_GL_FUNC)
        LIST_GLES2_EXTENSIONS_FUNCTIONS(LOAD_GLEXT_FUNC)
    } else if (version == GLES_2_0){
        LIST_GLES2_ONLY_FUNCTIONS(LOAD_GL_FUNC)
        LIST_GLES2_EXTENSIONS_FUNCTIONS(LOAD_GLEXT_FUNC)
    }

    /* Load OpenGL ES 3.x functions through 3.1. Not all are supported;
     * leave it up to EGL to determine support level. */
    LIST_GLES3_ONLY_FUNCTIONS(LOAD_GLEXT_FUNC)
    LIST_GLES31_ONLY_FUNCTIONS(LOAD_GLEXT_FUNC)

    if (s_got_gles_support_level) return;

    // Now detect the maximum level of GLES 3.x support from the host system.
    bool gles30_supported = true;
    bool gles31_supported = true;
    bool gles32_supported = false;
    // For 3.0, we don't really need glInvalidate(Sub)Framebuffer.
#define DETECT_GLES30_SUPPORT(return_type, function_name, signature, callargs) do { \
    if (!function_name && \
        strcmp(#function_name, "glInvalidateFramebuffer") && \
        strcmp(#function_name, "glInvalidateSubFramebuffer") ) { \
        gles30_supported = false; \
    } \
    } while(0); \

    LIST_GLES3_ONLY_FUNCTIONS(DETECT_GLES30_SUPPORT)

#define DETECT_GLES31_SUPPORT(return_type, function_name, signature, callargs) do { \
    if (!function_name) { \
        gles31_supported = false; } \
    } while(0); \

    LIST_GLES31_ONLY_FUNCTIONS(DETECT_GLES31_SUPPORT)

    if (gles30_supported && gles31_supported && gles32_supported) {
        s_max_supported_gles_version = GL_DISPATCH_MAX_GLES_VERSION_3_2;
    } else if (gles30_supported && gles31_supported) {
        s_max_supported_gles_version = GL_DISPATCH_MAX_GLES_VERSION_3_1;
    } else if (gles30_supported) {
        s_max_supported_gles_version = GL_DISPATCH_MAX_GLES_VERSION_3_0;
    } else {
        s_max_supported_gles_version = GL_DISPATCH_MAX_GLES_VERSION_2;
    }

    m_isLoaded = true;
    s_got_gles_support_level = true;
}

extern "C" GL_APICALL GLDispatchMaxGLESVersion GL_APIENTRY gl_dispatch_get_max_version() {
    return s_max_supported_gles_version;
}
