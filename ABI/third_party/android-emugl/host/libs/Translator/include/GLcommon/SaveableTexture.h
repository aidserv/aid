/*
* Copyright (C) 2017 The Android Open Source Project
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

#include "android/base/files/Stream.h"
#include "GLcommon/NamedObject.h"
#include "GLcommon/TextureData.h"
#include "GLcommon/TranslatorIfaces.h"

#include <memory>

class GLDispatch;
class GlobalNameSpace;

// TextureGlobal is an auxiliary class when save and load a texture / EglImage.
// We need it only for texture because texture is the only GL object that can be
// shared globally (i.e. across guest processes) by wrapping it up in EglImage.

// Lifespan:
// When saving a snapshot, TextureGlobal should be populated in the pre-save
// stage, save data in save stage and get destroyed right after that.
// When loading from a snapshot, TextureGlobal will be populated and restoring
// hardware texture before restoring any EglImage or GLES texture handles.
// EglImages and GLES textures get their global handles from TextureGlobal when
// EglImages and GLES textures are being loaded. Then TextureGlobal will be
// destroyed.

class SaveableTexture {
public:
    typedef std::function<void(SaveableTexture*, android::base::Stream*)> saver_t;
    typedef std::function<SaveableTexture*(android::base::Stream*,
            GlobalNameSpace*)> loader_t;

    SaveableTexture() = default;
    SaveableTexture(SaveableTexture&&) = default;
    SaveableTexture& operator=(SaveableTexture&&) = default;
    SaveableTexture(const EglImage& eglImage);
    SaveableTexture(const TextureData& texture);
    // precondition: a context must be properly bound
    SaveableTexture(android::base::Stream* stream,
        GlobalNameSpace* globalNameSpace);
    // precondition: a context must be properly bound
    void onSave(android::base::Stream* stream) const;
    NamedObjectPtr getGlobalObject() const;
    EglImage* makeEglImage() const;
    // TODO: makeTextureData as well
private:
    unsigned int m_target = GL_TEXTURE_2D;
    unsigned int m_width = 0;
    unsigned int m_height = 0;
    unsigned int m_depth = 0; // For texture 3D
    unsigned int m_format = GL_RGBA;
    unsigned int m_internalFormat = GL_RGBA;
    unsigned int m_type = GL_UNSIGNED_BYTE;
    unsigned int m_border = 0;
    // Attributes used when saving a snapshot
    unsigned int m_globalName = 0;
    // Attributes used when loaded from a snapshot
    NamedObjectPtr m_globalTexObj = nullptr;
};

typedef std::shared_ptr<SaveableTexture> SaveableTexturePtr;
