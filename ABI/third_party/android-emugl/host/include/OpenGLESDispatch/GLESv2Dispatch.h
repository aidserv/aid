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
#pragma once

#include "OpenGLESDispatch/gldefs.h"
#include "OpenGLESDispatch/gles_functions.h"
#include "KHR/khrplatform.h"

// Define function pointer types.
#define GLES2_DISPATCH_DEFINE_TYPE(return_type,func_name,signature,callargs) \
    typedef return_type (KHRONOS_APIENTRY * func_name ## _t) signature;

LIST_GLES2_FUNCTIONS(GLES2_DISPATCH_DEFINE_TYPE,GLES2_DISPATCH_DEFINE_TYPE)

struct GLESv2Dispatch {
#define GLES2_DISPATCH_DECLARE_POINTER(return_type,func_name,signature,callargs) \
        func_name ## _t func_name;
    LIST_GLES2_FUNCTIONS(GLES2_DISPATCH_DECLARE_POINTER,
                         GLES2_DISPATCH_DECLARE_POINTER)
};

#undef GLES2_DISPATCH_DECLARE_POINTER
#undef GLES2_DISPATCH_DEFINE_TYPE

bool gles2_dispatch_init(GLESv2Dispatch* dispatch_table);

// Used to initialize the decoder.
void* gles2_dispatch_get_proc_func(const char* name, void* userData);
// Used to check for unimplemented.
void gles2_unimplemented();

// Used to query max GLES version support based on what the dispatch mechanism
// has found in the GLESv2 library.
// First, a enum for tracking the detected GLES version based on dispatch.
// We support 2 minimally.
enum GLESDispatchMaxVersion {
    GLES_DISPATCH_MAX_VERSION_2 = 0,
    GLES_DISPATCH_MAX_VERSION_3_0 = 1,
    GLES_DISPATCH_MAX_VERSION_3_1 = 2,
    GLES_DISPATCH_MAX_VERSION_3_2 = 3,
};

GLESDispatchMaxVersion gles2_dispatch_get_max_version();
