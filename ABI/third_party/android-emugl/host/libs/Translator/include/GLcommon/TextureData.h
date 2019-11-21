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
#include "GLcommon/ObjectData.h"

#include <unordered_map>
#include <vector>

class TextureData : public ObjectData
{
public:
    TextureData():  ObjectData(TEXTURE_DATA),
                    width(0),
                    height(0),
                    depth(0),
                    border(0),
                    internalFormat(GL_RGBA),
                    format(GL_RGBA),
                    type(GL_UNSIGNED_BYTE),
                    sourceEGLImage(0),
                    hasStorage(false),
                    wasBound(false),
                    requiresAutoMipmap(false),
                    compressed(false),
                    compressedFormat(0),
                    target(0) {
        memset(crop_rect,0,4*sizeof(int));
    };
    TextureData(android::base::Stream* stream);

    unsigned int width;
    unsigned int height;
    unsigned int depth;
    unsigned int border;
    unsigned int internalFormat;
    // TODO: store emulated internal format
    unsigned int format;
    unsigned int type;
    unsigned int sourceEGLImage;
    bool hasStorage;
    bool wasBound;
    bool requiresAutoMipmap;
    bool compressed;
    unsigned int compressedFormat;
    int          crop_rect[4];
    GLenum target;
    // globalName is used for snapshot when reading data from GPU
    int globalName = 0;
    virtual void onSave(android::base::Stream* stream) const override;
    virtual void restore(ObjectLocalName localName,
            getGlobalName_t getGlobalName) override;
    void setTexParam(GLenum pname, GLint param);
protected:
    std::unordered_map<GLenum, GLint> m_texParam;
    // loadedData is a temporary buffer used between loading from a snapshot
    // and restoring hardware GPU states
    std::vector<unsigned char> loadedData;
};
