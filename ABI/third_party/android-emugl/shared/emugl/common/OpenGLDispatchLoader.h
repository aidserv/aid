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

#pragma once

//#include "android/base/system/System.h"

#include "OpenGLESDispatch/EGLDispatch.h"
#include "OpenGLESDispatch/GLESv2Dispatch.h"

// Helper class to hold a global GLESv2Dispatch that is initialized lazily
// in a thread-safe way. The instance is leaked on program exit.
struct LazyLoadedGLESv2Dispatch : public GLESv2Dispatch {
    // Return pointer to global GLESv2Dispatch instance, or nullptr if there
    // was an error when trying to initialize/load the library.
    static const GLESv2Dispatch* get();

    LazyLoadedGLESv2Dispatch() {
        mValid = gles2_dispatch_init(&mDispatch);
    }

private:
    GLESv2Dispatch mDispatch;
    bool mValid;
};

// Helper class used to lazily initialize the global EGL dispatch table
// in a thread safe way. Note that the dispatch table is provided by
// libOpenGLESDispatch as the 's_egl' global variable.
struct LazyLoadedEGLDispatch : public EGLDispatch {
    // Return pointer to EGLDispatch table, or nullptr if there was
    // an error when trying to initialize/load the library.
    static const EGLDispatch* get();

    LazyLoadedEGLDispatch() { mValid = init_egl_dispatch(); }

private:
    bool mValid;
};
