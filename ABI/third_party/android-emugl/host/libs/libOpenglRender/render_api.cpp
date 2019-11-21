/*
* Copyright (C) 2011-2015 The Android Open Source Project
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
#include "OpenglRender/render_api.h"

#include "ErrorLog.h"
#include "OpenGLESDispatch/EGLDispatch.h"
#include "OpenGLESDispatch/GLESv1Dispatch.h"
#include "OpenGLESDispatch/GLESv2Dispatch.h"
#include "RenderLibImpl.h"
#if 1
#include "OpenglRender/opengles_info.h"
#include "RenderControl.h"
#endif
#include <memory>

GLESv2Dispatch s_gles2;
GLESv1Dispatch s_gles1;



RENDER_APICALL emugl::RenderLib* RENDER_APIENTRY initLibrary()
{
    //
    // Load EGL Plugin
    //
    if (!init_egl_dispatch()) {
        // Failed to load EGL
        printf("Failed to init_egl_dispatch\n");
        return nullptr;
    }

    //
    // Load GLES Plugin
    //
    if (!gles1_dispatch_init(&s_gles1)) {
        // Failed to load GLES
        ERR("Failed to gles1_dispatch_init\n");
        return nullptr;
    }

    /* failure to init the GLES2 dispatch table is not fatal */
    if (!gles2_dispatch_init(&s_gles2)) {
        ERR("Failed to gles2_dispatch_init\n");
        return nullptr;
    }
	// s_gles1.initDispatchByName(gles1_dispatch_get_proc_func, nullptr);
	// s_gles2.initDispatchByName(gles2_dispatch_get_proc_func, nullptr);

    // return emugl::RenderLibPtr(new emugl::RenderLibImpl());
    return (emugl::RenderLib*)new emugl::RenderLibImpl();
}
#include "RenderServer.h"
// 启动Tcp接收端服务
RENDER_APICALL int RENDER_APIENTRY StartOpenglServer(char* addr, size_t addrLen)
{
#if 1
    RenderServer *s_renderThread = RenderServer::create(addr, addrLen);
    if (!s_renderThread) {
        return false;
    }

    s_renderThread->start();

#endif
    
    
    return true;
}

