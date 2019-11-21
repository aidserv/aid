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
#ifndef EGL_CONTEXT_H
#define EGL_CONTEXT_H

#include "android/base/files/Stream.h"

#include "EglConfig.h"
#include "EglOsApi.h"
#include "EglSurface.h"

#include "emugl/common/smart_ptr.h"

#include "GLcommon/GLutils.h"
#include "GLcommon/TranslatorIfaces.h"
#include "GLcommon/ShareGroup.h"

#include <EGL/egl.h>

class EglContext;
typedef emugl::SmartPtr<EglContext> ContextPtr;

class EglDisplay;

class EglContext {
public:
    EglContext(EglDisplay* dpy,
               uint64_t shareGroupId,
               EglConfig* config,
               GLEScontext* glesCtx,
               GLESVersion ver,
               ObjectNameManager* mngr,
               android::base::Stream* stream);
    bool usingSurface(SurfacePtr surface);
    EglOS::Context* nativeType() const { return m_native; }
    bool getAttrib(EGLint attrib, EGLint* value);
    const SurfacePtr& read() const { return m_read; };
    const SurfacePtr& draw() const { return m_draw; };
    const ShareGroupPtr& getShareGroup() const { return m_shareGroup; }
    EglConfig* getConfig() { return m_config; };
    GLESVersion version() { return m_version; };
    GLEScontext* getGlesContext() { return m_glesContext; }
    void setSurfaces(SurfacePtr read, SurfacePtr draw);
    unsigned int getHndl() { return m_hndl; }

    ~EglContext();
    void onSave(android::base::Stream* stream);
    void postSave(android::base::Stream* stream);
    // TODO: return an EglContext instead of simply reading back the data
    static bool onLoad(android::base::Stream* stream,
                       EGLint& configId,
                       uint64_t& shareGroupId);
private:
    static unsigned int s_nextContextHndl;
    EglDisplay* m_dpy = nullptr;
    EglOS::Context* m_native = nullptr;
    EglConfig* m_config = nullptr;
    GLEScontext* m_glesContext = nullptr;
    ShareGroupPtr m_shareGroup;
    SurfacePtr m_read;
    SurfacePtr m_draw;
    GLESVersion m_version;
    ObjectNameManager* m_mngr = nullptr;
    unsigned int m_hndl = 0;
};

#endif
