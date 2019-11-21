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
#include "EglGlobalInfo.h"

#include "ClientAPIExts.h"
#include "EglDisplay.h"
#include "EglOsApi.h"

#include "emugl/common/lazy_instance.h"

#include <string.h>

namespace {

// Use a LazyInstance to ensure thread-safe initialization.
emugl::LazyInstance<EglGlobalInfo> sSingleton = LAZY_INSTANCE_INIT;

}  // namespace

// static
EglGlobalInfo* EglGlobalInfo::getInstance() {
    return sSingleton.ptr();
}

EglGlobalInfo::EglGlobalInfo() {
    // TODO(digit): Choose alternate engine based on env. variable?
    m_engine = EglOS::Engine::getHostInstance();
    m_display = m_engine->getDefaultDisplay();
}

EglGlobalInfo::~EglGlobalInfo() {
    for (size_t n = 0; n < m_displays.size(); ++n) {
        delete m_displays[n];
    }
}

EglDisplay* EglGlobalInfo::addDisplay(EGLNativeDisplayType dpy,
                                      EglOS::Display* idpy) {
    //search if it already exists.
    emugl::Mutex::AutoLock mutex(m_lock);
    for (size_t n = 0; n < m_displays.size(); ++n) {
        if (m_displays[n]->getNativeDisplay() == dpy) {
            return m_displays[n];
        }
    }

    if (!idpy) {
        return NULL;
    }
    EglDisplay* result = new EglDisplay(dpy, idpy);
    m_displays.push_back(result);
    return result;
}

bool  EglGlobalInfo::removeDisplay(EGLDisplay dpy) {
    emugl::Mutex::AutoLock mutex(m_lock);
    for (size_t n = 0; n < m_displays.size(); ++n) {
        if (m_displays[n] == static_cast<EglDisplay*>(dpy)) {
            delete m_displays[n];
            m_displays.erase(m_displays.begin() + n);
            return true;
        }
    }
    return false;
}

EglDisplay* EglGlobalInfo::getDisplay(EGLNativeDisplayType dpy) const {
    emugl::Mutex::AutoLock mutex(m_lock);
    for (size_t n = 0; n < m_displays.size(); ++n) {
        if (m_displays[n]->getNativeDisplay() == dpy) {
            return m_displays[n];
        }
    }
    return NULL;
}

EglDisplay* EglGlobalInfo::getDisplay(EGLDisplay dpy) const {
    emugl::Mutex::AutoLock mutex(m_lock);
    for (size_t n = 0; n < m_displays.size(); ++n) {
        if (m_displays[n] == static_cast<EglDisplay*>(dpy)) {
            return m_displays[n];
        }
    }
    return NULL;
}

void EglGlobalInfo::initClientExtFuncTable(GLESVersion ver) {
    emugl::Mutex::AutoLock mutex(m_lock);
    if (!m_gles_extFuncs_inited[ver]) {
        ClientAPIExts::initClientFuncs(m_gles_ifaces[ver], (int)ver - 1);
        m_gles_extFuncs_inited[ver] = true;
    }
}

void EglGlobalInfo::markSurfaceForDestroy(EglDisplay* display,
                                          EGLSurface toDestroy) {
    emugl::Mutex::AutoLock mutex(m_lock);
    assert(display);
    m_surfaceDestroyList.push_back(
        std::make_pair(display, toDestroy));
}

void EglGlobalInfo::sweepDestroySurfaces() {
    emugl::Mutex::AutoLock mutex(m_lock);
    for (auto elt : m_surfaceDestroyList) {
        EglDisplay* dpy = elt.first;
        assert(dpy);
        EGLSurface surface = elt.second;
        SurfacePtr surfacePtr = dpy->getSurface(surface);
        if (surfacePtr) {
            m_gles_ifaces[GLES_2_0]->deleteRbo(surfacePtr->glRboColor);
            m_gles_ifaces[GLES_2_0]->deleteRbo(surfacePtr->glRboDepth);
        }
        dpy->removeSurface(surface);
    }
    m_surfaceDestroyList.clear();
}
