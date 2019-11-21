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
#include "EglDisplay.h"

#include "android/base/containers/Lookup.h"
#include "android/base/files/StreamSerializing.h"
#include "EglConfig.h"
#include "EglOsApi.h"
#include <GLcommon/GLutils.h>

#include <algorithm>

EglDisplay::EglDisplay(EGLNativeDisplayType dpy,
                       EglOS::Display* idpy) :
    m_dpy(dpy),
    m_idpy(idpy)
{
    m_manager[GLES_1_1] = new ObjectNameManager(&m_globalNameSpace);
    m_manager[GLES_2_0] = new ObjectNameManager(&m_globalNameSpace);
    m_manager[GLES_3_0] = m_manager[GLES_2_0];
    m_manager[GLES_3_1] = m_manager[GLES_2_0];
};

EglDisplay::~EglDisplay() {
    emugl::Mutex::AutoLock mutex(m_lock);

    //
    // Destroy the global context if one was created.
    // (should be true for windows platform only)
    //
    if (m_globalSharedContext != NULL) {
        m_idpy->destroyContext(m_globalSharedContext);
    }

    m_configs.clear();

    delete m_manager[GLES_1_1];
    delete m_manager[GLES_2_0];

    delete m_idpy;
}

void EglDisplay::initialize(int renderableType) {
    emugl::Mutex::AutoLock mutex(m_lock);
    m_initialized = true;
    initConfigurations(renderableType);
    m_configInitialized = true;
}

bool EglDisplay::isInitialize() { return m_initialized;}

void EglDisplay::terminate(){
    emugl::Mutex::AutoLock mutex(m_lock);
     m_contexts.clear();
     m_surfaces.clear();
     m_initialized = false;
}

namespace CompareEglConfigs {

// Old compare function used to initialize to something decently sorted.
struct StaticCompare {
    bool operator()(const std::unique_ptr<EglConfig>& first,
                    const std::unique_ptr<EglConfig>& second) const {
        return *first < *second;
    }
};

// In actual usage, we need to dynamically re-sort configs
// that are returned to the user.
struct DynamicCompare;
// This is because the sorting order of configs is affected
// based on dynamic properties.
//
// See https://www.khronos.org/registry/egl/sdk/docs/man/html/eglChooseConfig.xhtml
// and the section on config sorting.
//
// If the user requests an EGL config with a particular EGL_RED_SIZE,
// for example, we must sort configs based on that criteria, while if that
// was not specified, we would just skip right on to sorting by buffer size.
// Below is an implementation of EGL config sorting according
// to spec, that takes the dynamic properties into account.
static int ColorBufferTypeVal(EGLenum type) {
    switch (type) {
    case EGL_RGB_BUFFER: return 0;
    case EGL_LUMINANCE_BUFFER: return 1;
    case EGL_YUV_BUFFER_EXT: return 2;
    }
    return 3;
}

static bool nonTrivialAttribVal(EGLint val) {
    return val != 0 && val != EGL_DONT_CARE;
}

struct DynamicCompare {
public:
    DynamicCompare(const EglConfig& wantedAttribs) {

        EGLint wantedRVal = wantedAttribs.getConfAttrib(EGL_RED_SIZE);
        EGLint wantedGVal = wantedAttribs.getConfAttrib(EGL_GREEN_SIZE);
        EGLint wantedBVal = wantedAttribs.getConfAttrib(EGL_BLUE_SIZE);
        EGLint wantedLVal = wantedAttribs.getConfAttrib(EGL_LUMINANCE_SIZE);
        EGLint wantedAVal = wantedAttribs.getConfAttrib(EGL_ALPHA_SIZE);

        wantedR = wantedAttribs.isWantedAttrib(EGL_RED_SIZE) && nonTrivialAttribVal(wantedRVal);
        wantedG = wantedAttribs.isWantedAttrib(EGL_GREEN_SIZE) && nonTrivialAttribVal(wantedGVal);
        wantedB = wantedAttribs.isWantedAttrib(EGL_BLUE_SIZE) && nonTrivialAttribVal(wantedBVal);
        wantedL = wantedAttribs.isWantedAttrib(EGL_LUMINANCE_SIZE) && nonTrivialAttribVal(wantedLVal);
        wantedA = wantedAttribs.isWantedAttrib(EGL_ALPHA_SIZE) && nonTrivialAttribVal(wantedAVal);
    }

    bool operator()(EglConfig* a, EglConfig* b) const {
        EGLint aConformant = a->getConfAttrib(EGL_CONFORMANT);
        EGLint bConformant = b->getConfAttrib(EGL_CONFORMANT);

        if (aConformant != bConformant) {
            return aConformant != 0;
        }

        EGLint aCaveat = a->getConfAttrib(EGL_CONFIG_CAVEAT);
        EGLint bCaveat = b->getConfAttrib(EGL_CONFIG_CAVEAT);
        if (aCaveat != bCaveat) {
            return aCaveat < bCaveat;
        }

        EGLint aCbType = a->getConfAttrib(EGL_COLOR_BUFFER_TYPE);
        EGLint bCbType = b->getConfAttrib(EGL_COLOR_BUFFER_TYPE);
        if (aCbType != bCbType) {
            return ColorBufferTypeVal(aCbType) <
                   ColorBufferTypeVal(bCbType);
        }

        EGLint aCbSize = 0;
        EGLint bCbSize = 0;

        if (wantedR) {
            aCbSize += a->getConfAttrib(EGL_RED_SIZE);
            bCbSize += b->getConfAttrib(EGL_RED_SIZE);
        }
        if (wantedG) {
            aCbSize += a->getConfAttrib(EGL_GREEN_SIZE);
            bCbSize += b->getConfAttrib(EGL_GREEN_SIZE);
        }
        if (wantedB) {
            aCbSize += a->getConfAttrib(EGL_BLUE_SIZE);
            bCbSize += b->getConfAttrib(EGL_BLUE_SIZE);
        }
        if (wantedL) {
            aCbSize += a->getConfAttrib(EGL_LUMINANCE_SIZE);
            bCbSize += b->getConfAttrib(EGL_LUMINANCE_SIZE);
        }
        if (wantedA) {
            aCbSize += a->getConfAttrib(EGL_ALPHA_SIZE);
            bCbSize += b->getConfAttrib(EGL_ALPHA_SIZE);
        }

        if (aCbSize != bCbSize) {
            return aCbSize > bCbSize;
        }

        EGLint aBufferSize = a->getConfAttrib(EGL_BUFFER_SIZE);
        EGLint bBufferSize = b->getConfAttrib(EGL_BUFFER_SIZE);
        if (aBufferSize != bBufferSize) {
            return aBufferSize < bBufferSize;
        }

        EGLint aSampleBuffersNum = a->getConfAttrib(EGL_SAMPLE_BUFFERS);
        EGLint bSampleBuffersNum = b->getConfAttrib(EGL_SAMPLE_BUFFERS);
        if (aSampleBuffersNum != bSampleBuffersNum) {
            return aSampleBuffersNum < bSampleBuffersNum;
        }

        EGLint aSPP = a->getConfAttrib(EGL_SAMPLES);
        EGLint bSPP = b->getConfAttrib(EGL_SAMPLES);
        if (aSPP != bSPP) {
            return aSPP < bSPP;
        }

        EGLint aDepthSize = a->getConfAttrib(EGL_DEPTH_SIZE);
        EGLint bDepthSize = b->getConfAttrib(EGL_DEPTH_SIZE);
        if (aDepthSize != bDepthSize) {
            return aDepthSize < bDepthSize;
        }

        EGLint aStencilSize = a->getConfAttrib(EGL_STENCIL_SIZE);
        EGLint bStencilSize = b->getConfAttrib(EGL_STENCIL_SIZE);
        if (aStencilSize != bStencilSize) {
            return aStencilSize < bStencilSize;
        }

        return a->getConfAttrib(EGL_CONFIG_ID) < b->getConfAttrib(EGL_CONFIG_ID);
    }

    bool wantedR;
    bool wantedG;
    bool wantedB;
    bool wantedL;
    bool wantedA;
};

}

void EglDisplay::addSimplePixelFormat(int red_size,
                                      int green_size,
                                      int blue_size,
                                      int alpha_size) {
    std::sort(m_configs.begin(), m_configs.end(), CompareEglConfigs::StaticCompare());

    EGLConfig match;

    EglConfig dummy(red_size,
                    green_size,
                    blue_size,
                    alpha_size,  // RGB_565
                    EGL_DONT_CARE,
                    EGL_DONT_CARE,
                    16, // Depth
                    EGL_DONT_CARE,
                    EGL_DONT_CARE,
                    EGL_DONT_CARE,
                    EGL_DONT_CARE,
                    EGL_DONT_CARE,
                    EGL_DONT_CARE,
                    EGL_DONT_CARE,
                    EGL_DONT_CARE,
                    EGL_DONT_CARE,
                    EGL_DONT_CARE,
                    EGL_DONT_CARE,
                    EGL_DONT_CARE,
                    EGL_DONT_CARE,
                    EGL_DONT_CARE,
                    EGL_DONT_CARE,
                    EGL_DONT_CARE,
                    NULL);

    if(!doChooseConfigs(dummy, &match, 1))
    {
        return;
    }

    const EglConfig* config = (EglConfig*)match;

    int bSize;
    config->getConfAttrib(EGL_BUFFER_SIZE,&bSize);

    if(bSize == 16)
    {
        return;
    }

    int max_config_id = 0;

    for(ConfigsList::iterator it = m_configs.begin(); it != m_configs.end() ;++it) {
        EGLint id;
        (*it)->getConfAttrib(EGL_CONFIG_ID, &id);
        if(id > max_config_id)
            max_config_id = id;
    }

    std::unique_ptr<EglConfig> newConfig(
         new EglConfig(*config,max_config_id+1,
                       red_size, green_size, blue_size,
                       alpha_size));

    if (m_uniqueConfigs.insert(*newConfig).second) {
        m_configs.emplace_back(newConfig.release());
    }
}

void EglDisplay::addMissingConfigs() {
    addSimplePixelFormat(5, 6, 5, 0); // RGB_565
    addSimplePixelFormat(8, 8, 8, 0); // RGB_888
    // (Host GPUs that are newer may not list RGB_888
    // out of the box.)
}

void EglDisplay::initConfigurations(int renderableType) {
    if (m_configInitialized) {
        return;
    }
    m_idpy->queryConfigs(renderableType, addConfig, this);

    addMissingConfigs();
    std::sort(m_configs.begin(), m_configs.end(), CompareEglConfigs::StaticCompare());

#if EMUGL_DEBUG
    for (ConfigsList::const_iterator it = m_configs.begin();
         it != m_configs.end();
         ++it) {
        EglConfig* config = it->get();
        EGLint red, green, blue, alpha, depth, stencil, renderable, surface;
        config->getConfAttrib(EGL_RED_SIZE, &red);
        config->getConfAttrib(EGL_GREEN_SIZE, &green);
        config->getConfAttrib(EGL_BLUE_SIZE, &blue);
        config->getConfAttrib(EGL_ALPHA_SIZE, &alpha);
        config->getConfAttrib(EGL_DEPTH_SIZE, &depth);
        config->getConfAttrib(EGL_STENCIL_SIZE, &stencil);
        config->getConfAttrib(EGL_RENDERABLE_TYPE, &renderable);
        config->getConfAttrib(EGL_SURFACE_TYPE, &surface);
    }
#endif  // EMUGL_DEBUG
}

EglConfig* EglDisplay::getConfig(EGLConfig conf) const {
    emugl::Mutex::AutoLock mutex(m_lock);

    for(ConfigsList::const_iterator it = m_configs.begin();
        it != m_configs.end();
        ++it) {
        if(static_cast<EGLConfig>(it->get()) == conf) {
            return it->get();
        }
    }
    return NULL;
}

SurfacePtr EglDisplay::getSurface(EGLSurface surface) const {
    emugl::Mutex::AutoLock mutex(m_lock);
    /* surface is "key" in map<unsigned int, SurfacePtr>. */
    unsigned int hndl = SafeUIntFromPointer(surface);
    SurfacesHndlMap::const_iterator it = m_surfaces.find(hndl);
    return it != m_surfaces.end() ?
                                  (*it).second :
                                   SurfacePtr();
}

ContextPtr EglDisplay::getContext(EGLContext ctx) const {
    emugl::Mutex::AutoLock mutex(m_lock);
    /* ctx is "key" in map<unsigned int, ContextPtr>. */
    unsigned int hndl = SafeUIntFromPointer(ctx);
    ContextsHndlMap::const_iterator it = m_contexts.find(hndl);
    return it != m_contexts.end() ?
                                  (*it).second :
                                   ContextPtr();
}

bool EglDisplay::removeSurface(EGLSurface s) {
    emugl::Mutex::AutoLock mutex(m_lock);
    /* s is "key" in map<unsigned int, SurfacePtr>. */
    unsigned int hndl = SafeUIntFromPointer(s);
    SurfacesHndlMap::iterator it = m_surfaces.find(hndl);
    if(it != m_surfaces.end()) {
        m_surfaces.erase(it);
        return true;
    }
    return false;
}

bool EglDisplay::removeContext(EGLContext ctx) {
    emugl::Mutex::AutoLock mutex(m_lock);
    /* ctx is "key" in map<unsigned int, ContextPtr>. */
    unsigned int hndl = SafeUIntFromPointer(ctx);
    ContextsHndlMap::iterator it = m_contexts.find(hndl);
    if(it != m_contexts.end()) {
        m_contexts.erase(it);
        return true;
    }
    return false;
}

bool EglDisplay::removeContext(ContextPtr ctx) {
    emugl::Mutex::AutoLock mutex(m_lock);

    ContextsHndlMap::iterator it;
    for(it = m_contexts.begin(); it != m_contexts.end();++it) {
        if((*it).second.get() == ctx.get()){
            break;
        }
    }
    if(it != m_contexts.end()) {
        m_contexts.erase(it);
        return true;
    }
    return false;
}

EglConfig* EglDisplay::getConfig(EGLint id) const {
    emugl::Mutex::AutoLock mutex(m_lock);

    for(ConfigsList::const_iterator it = m_configs.begin();
        it != m_configs.end();
        ++it) {
        if((*it)->id() == id) {
            return it->get();
        }
    }
    return NULL;
}

int EglDisplay::getConfigs(EGLConfig* configs,int config_size) const {
    emugl::Mutex::AutoLock mutex(m_lock);
    int i = 0;
    for(ConfigsList::const_iterator it = m_configs.begin();
        it != m_configs.end() && i < config_size;
        i++, ++it) {
        configs[i] = static_cast<EGLConfig>(it->get());
    }
    return i;
}

int EglDisplay::chooseConfigs(const EglConfig& dummy,
                              EGLConfig* configs,
                              int config_size) const {
    emugl::Mutex::AutoLock mutex(m_lock);
    return doChooseConfigs(dummy, configs, config_size);
}

int EglDisplay::doChooseConfigs(const EglConfig& dummy,
                                EGLConfig* configs,
                                int config_size) const {
    int added = 0;

    std::vector<EglConfig*> validConfigs;

    CHOOSE_CONFIG_DLOG("returning configs. ids: {");
    for(ConfigsList::const_iterator it = m_configs.begin();
        it != m_configs.end() && (added < config_size || !configs);
        ++it) {
        if( (*it)->chosen(dummy)){
            if(configs) {
                CHOOSE_CONFIG_DLOG("valid config: id=0x%x", it->get()->id());
                validConfigs.push_back(it->get());
            }
            added++;
       }
    }

    CHOOSE_CONFIG_DLOG("sorting valid configs...");

    std::sort(validConfigs.begin(),
              validConfigs.end(),
              CompareEglConfigs::DynamicCompare(dummy));

    for (int i = 0; i < added; i++) {
        configs[i] = static_cast<EGLConfig>(validConfigs[i]);
    }

    CHOOSE_CONFIG_DLOG("returning configs. ids end }");
    return added;
}

EGLSurface EglDisplay::addSurface(SurfacePtr s ) {
   emugl::Mutex::AutoLock mutex(m_lock);
   unsigned int hndl = s.get()->getHndl();
   EGLSurface ret =reinterpret_cast<EGLSurface> (hndl);

   if(m_surfaces.find(hndl) != m_surfaces.end()) {
       return ret;
   }

   m_surfaces[hndl] = s;
   return ret;
}

EGLContext EglDisplay::addContext(ContextPtr ctx ) {
    emugl::Mutex::AutoLock mutex(m_lock);

   unsigned int hndl = ctx.get()->getHndl();
   EGLContext ret    = reinterpret_cast<EGLContext> (hndl);

   if(m_contexts.find(hndl) != m_contexts.end()) {
       return ret;
   }
   m_contexts[hndl] = ctx;
   return ret;
}


EGLImageKHR EglDisplay::addImageKHR(ImagePtr img) {
    emugl::Mutex::AutoLock mutex(m_lock);
    do {
        ++m_nextEglImageId;
    } while(m_nextEglImageId == 0
            || android::base::contains(m_eglImages, m_nextEglImageId));
    img->imageId = m_nextEglImageId;
    m_eglImages[m_nextEglImageId] = img;
    return reinterpret_cast<EGLImageKHR>(m_nextEglImageId);
}

ImagePtr EglDisplay::getImage(EGLImageKHR img) const {
    emugl::Mutex::AutoLock mutex(m_lock);
    /* img is "key" in map<unsigned int, ImagePtr>. */
    unsigned int hndl = SafeUIntFromPointer(img);
    ImagesHndlMap::const_iterator i( m_eglImages.find(hndl) );
    return (i != m_eglImages.end()) ? (*i).second :ImagePtr();
}

bool EglDisplay:: destroyImageKHR(EGLImageKHR img) {
    emugl::Mutex::AutoLock mutex(m_lock);
    /* img is "key" in map<unsigned int, ImagePtr>. */
    unsigned int hndl = SafeUIntFromPointer(img);
    ImagesHndlMap::iterator i( m_eglImages.find(hndl) );
    if (i != m_eglImages.end())
    {
        m_eglImages.erase(i);
        return true;
    }
    return false;
}

EglOS::Context* EglDisplay::getGlobalSharedContext() const {
    emugl::Mutex::AutoLock mutex(m_lock);
#ifndef _WIN32
    // find an existing OpenGL context to share with, if exist
    EglOS::Context* ret =
        (EglOS::Context*)m_manager[GLES_1_1]->getGlobalContext();
    if (!ret)
        ret = (EglOS::Context*)m_manager[GLES_2_0]->getGlobalContext();
    return ret;
#else
    if (!m_globalSharedContext) {
        //
        // On windows we create a dummy context to serve as the
        // "global context" which all contexts share with.
        // This is because on windows it is not possible to share
        // with a context which is already current. This dummy context
        // will never be current to any thread so it is safe to share with.
        // Create that context using the first config
        if (m_configs.empty()) {
            // Should not happen! config list should be initialized at this point
            return NULL;
        }
        EglConfig *cfg = m_configs.front().get();
        m_globalSharedContext = m_idpy->createContext(
                cfg->nativeFormat(), NULL);
    }

    return m_globalSharedContext;
#endif
}

// static
void EglDisplay::addConfig(void* opaque, const EglOS::ConfigInfo* info) {
    EglDisplay* display = static_cast<EglDisplay*>(opaque);

    // Greater than 24 bits of color,
    // or having no depth/stencil causes some
    // unexpected behavior in real usage, such
    // as frame corruption and wrong drawing order.
    // Also, disallow high MSAA.
    // Just don't use those configs.
    if (info->red_size > 8 ||
        info->green_size > 8 ||
        info->blue_size > 8 ||
        info->depth_size < 24 ||
        info->stencil_size < 8 ||
        info->samples_per_pixel > 4) {
        return;
    }

    std::unique_ptr<EglConfig> config(new EglConfig(
         info->red_size,
         info->green_size,
         info->blue_size,
         info->alpha_size,
         info->caveat,
         info->config_id,
         info->depth_size,
         info->frame_buffer_level,
         info->max_pbuffer_width,
         info->max_pbuffer_height,
         info->max_pbuffer_size,
         info->native_renderable,
         info->renderable_type,
         info->native_visual_id,
         info->native_visual_type,
         info->samples_per_pixel,
         info->stencil_size,
         info->surface_type,
         info->transparent_type,
         info->trans_red_val,
         info->trans_green_val,
         info->trans_blue_val,
         info->recordable_android,
         info->frmt));

    if (display->m_uniqueConfigs.insert(*config).second) {
        display->m_configs.emplace_back(config.release());
    }
}

void EglDisplay::onSaveAllImages(android::base::Stream* stream,
        SaveableTexture::saver_t saver) {
    // we could consider calling presave for all ShareGroups from here
    // but it would introduce overheads because not all share groups need to be
    // saved
    emugl::Mutex::AutoLock mutex(m_lock);
    for (const auto& image : m_eglImages) {
        getGlobalNameSpace()->preSaveAddEglImage(image.second.get());
    }
    m_globalNameSpace.onSave(stream, saver);
    saveCollection(stream, m_eglImages, [](
            android::base::Stream* stream,
            const ImagesHndlMap::value_type& img) {
        stream->putBe32(img.first);
        stream->putBe32(img.second->globalTexObj->getGlobalName());
        // We do not need to save other fields in EglImage. We can load them
        // from SaveableTexture.
    });
}

void EglDisplay::onLoadAllImages(android::base::Stream* stream,
        SaveableTexture::loader_t loader) {
    if (!m_eglImages.empty()) {
        // Could be triggered by this bug:
        // b/36654917
        fprintf(stderr, "Warning: unreleased EGL image handles\n");
    }
    m_eglImages.clear();
    emugl::Mutex::AutoLock mutex(m_lock);
    m_globalNameSpace.onLoad(stream, loader);
    loadCollection(stream, &m_eglImages, [this](
        android::base::Stream* stream) {
        unsigned int hndl = stream->getBe32();
        unsigned int globalName = stream->getBe32();
        ImagePtr eglImg(m_globalNameSpace.makeEglImageFromLoad(globalName));
        eglImg->imageId = hndl;
        return std::make_pair(hndl, std::move(eglImg));
    });
}

void EglDisplay::postLoadAllImages(android::base::Stream* stream) {
    m_globalNameSpace.postLoad(stream);
}
