/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include "gles_context.h"
#include "egl_image.h"
#include "GLES/glext.h"

#include "common/dlog.h"
#include "emugl/common/lazy_instance.h"
#include "gles/macros.h"

#include "android/base/threads/ThreadStore.h"
#include "android/utils/debug.h"

#include "GLES12Translator/underlying_apis.h"
#include "GLES12Translator/angle_gles2.h"

#include <stdio.h>

// Interface to emulator's libOpenglRender.
// translator_interface provides all the essential functions for properly
// using the underlying EGL + GLESv2 libraries.
// The information requested by these interface functions
// must be available to libOpenglRender.

// The first part is handling all thread-local info.
// Each GLESv1 context requested by the guest will spawn a thread-local
// GlesContext* object that tracks all GLESv1->2 translation state.
// GLES12ThreadInfo and ThreadStore/LazyInstance capture this.

class GLES12ThreadInfo {
public:
    GLES12ThreadInfo();
    ~GLES12ThreadInfo();
    static GLES12ThreadInfo* get();
    GlesContext* context;
};

typedef android::base::ThreadStore<GLES12ThreadInfo> EmulatedGLES1ThreadInfo;

static ::emugl::LazyInstance<EmulatedGLES1ThreadInfo> sGLES12ThreadInfo = LAZY_INSTANCE_INIT;

GLES12ThreadInfo::GLES12ThreadInfo() {
    sGLES12ThreadInfo->set(this);
    context = NULL;
}

GLES12ThreadInfo::~GLES12ThreadInfo() {
    sGLES12ThreadInfo->set(NULL);
}

GLES12ThreadInfo* GLES12ThreadInfo::get() {
    GLES12ThreadInfo* current = static_cast<GLES12ThreadInfo*>(sGLES12ThreadInfo->get());
    if (!current) { current = new GLES12ThreadInfo; }
    return current;
}

// Utility functions used for the translator interface
// and throughout the GLESv1->2 translator as well.

GlesContext* CreateGles1Context(GlesContext* share,
                                const UnderlyingApis* underlying_apis) {
    return new GlesContext(0, kGles11, share, NULL, underlying_apis);
}

GlesContext* GetCurrentGlesContext() {
    DLOG("call");

    GLES12ThreadInfo* res = GLES12ThreadInfo::get();
    DLOG("res=%p rescxt=%p\n", res, res->context);
    return res->context;
}

void SetCurrentGlesContext(GlesContext* cxt_in) {
    GLES12ThreadInfo::get()->context = cxt_in;
}

// The actual translator interface entries.

TRANSLATOR_APIENTRY(void*, create_underlying_api) {
#if GLES12TR_DLOG
    VERBOSE_ENABLE(gles1emu);
#endif
    UnderlyingApis* container = new UnderlyingApis;
    container->angle = new ANGLE_GLES2;
    return (void*)container;
}

TRANSLATOR_APIENTRY(void*, create_gles1_context, void* share, const void* underlying_apis) {
    return (void*)CreateGles1Context((GlesContext*)share, (const UnderlyingApis*)underlying_apis);
}

TRANSLATOR_APIENTRY(void*, get_current_gles_context) {
    return (void*)GetCurrentGlesContext();
}

TRANSLATOR_APIENTRY(void, set_current_gles_context, void* cxt_in) {
    SetCurrentGlesContext((GlesContext*)cxt_in);
}

TRANSLATOR_APIENTRY(void, make_current_setup, GLuint width, GLuint height) {
    GlesContext* cxt = GetCurrentGlesContext();
    DLOG("width=%u height=%u cxt@%p", width, height, cxt);
    cxt->OnAttachSurface(NULL, width, height);
    cxt->OnMakeCurrent();
}

TRANSLATOR_APIENTRY(void, destroy_gles1_context, void* cxt) {
    GlesContext* toDelete = (GlesContext*)cxt;
    delete toDelete;
}

// Unsupported functions that are not absolutely critical to GLESv1->2 translation
// in the emulator, but will be nice to have in the future.
// We will need to implement these functions for compressed texture support.

void* MapTexSubImage2DCHROMIUMCall (const GlesContext* c,
                                    GLenum target, GLint level,
                                    GLint xoffset, GLint yoffset,
                                    GLsizei width, GLsizei height,
                                    GLenum format, GLenum type, GLenum access)
{
    fprintf(stderr, "%s called (unsupported!)", __FUNCTION__);
    return NULL;
}

void  UnmapTexSubImage2DCHROMIUMCall (const GlesContext* c, const void* mem)
{
    fprintf(stderr, "%s called (unsupported!)", __FUNCTION__);
    return;
}

void* MapBufferSubDataCHROMIUMCall (GLuint target, GLintptr offset,
                                    GLsizeiptr size, GLenum access)
{
    fprintf(stderr, "%s called (unsupported!)", __FUNCTION__);
    return NULL;
}

EglImagePtr GetEglImageFromNativeBuffer(GLeglImageOES buffer)
{
    fprintf(stderr, "%s called (unsupported!)", __FUNCTION__);
    return NULL;
    GLenum target = 0;
    GLuint texture = 0;
    return EglImage::Create(target, texture);
}
