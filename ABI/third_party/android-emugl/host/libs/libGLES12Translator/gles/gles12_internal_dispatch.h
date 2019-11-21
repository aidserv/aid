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

#pragma once

#include "OpenGLESDispatch/gldefs.h"
#include "OpenGLESDispatch/gles_functions.h"
#include "OpenGLESDispatch/GLESv1Dispatch.h"

// gles12tr_internal.h is to make it easier to do broad strokes
// with the functions defined in the GLESv1->2 translator,
// which is a different set of functions from the standard set of
// GLESv1 functions in libOpenGLESDispatch.
#include "OpenGLESDispatch/gles12tr_internal.h"

#include "KHR/khrplatform.h"

// Define function pointer types.
#define GLES1_DISPATCH_DEFINE_TYPE(return_type,func_name,signature,callargs) \
    typedef return_type (KHRONOS_APIENTRY * func_name ## _t) signature;

LIST_GLES12TR_INTERNAL_FUNCTIONS(GLES1_DISPATCH_DEFINE_TYPE)

struct GLES12TranslatorFuncs {
#define GLES1_DISPATCH_DECLARE_POINTER(return_type,func_name,signature,callargs) \
        func_name ## _t internal##func_name;
    LIST_GLES12TR_INTERNAL_FUNCTIONS(GLES1_DISPATCH_DECLARE_POINTER)
};

#undef GLES1_DISPATCH_DECLARE_POINTER
#undef GLES1_DISPATCH_DEFINE_TYPE
