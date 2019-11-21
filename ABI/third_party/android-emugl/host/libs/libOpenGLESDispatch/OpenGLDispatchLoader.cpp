// Copyright (C) 2017 The Android Open Source Project
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

#include "emugl/common/OpenGLDispatchLoader.h"
#include "OpenglCodecCommon/ProtocolUtils.h"
//#include "android/base/memory/LazyInstance.h"

// Must be declared outside of LazyLoadedGLESv2Dispatch scope due to the use of
// sizeof(T) within the template definition.
// android::base::LazyInstance<LazyLoadedGLESv2Dispatch> sGLESv2Dispatch = LAZY_INSTANCE_INIT;
LazyLoadedGLESv2Dispatch gGLESv2Dispatch;

// static
const GLESv2Dispatch* LazyLoadedGLESv2Dispatch::get() {
    LazyLoadedGLESv2Dispatch* instance = &gGLESv2Dispatch; // sGLESv2Dispatch.ptr();
    if (instance->mValid) {
        return &instance->mDispatch;
    } else {
        return nullptr;
    }
}

// android::base::LazyInstance<LazyLoadedEGLDispatch> sEGLDispatch = LAZY_INSTANCE_INIT;
LazyLoadedEGLDispatch gEGLDispatch;

// static
const EGLDispatch* LazyLoadedEGLDispatch::get() {
    LazyLoadedEGLDispatch* instance = &gEGLDispatch; // sEGLDispatch.ptr();
    if (instance->mValid) {
        return &s_egl;
    } else {
        return nullptr;
    }
}
