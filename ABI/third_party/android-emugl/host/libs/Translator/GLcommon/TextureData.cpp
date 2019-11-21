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

#include "GLcommon/TextureData.h"

#include "android/base/files/StreamSerializing.h"
#include "GLcommon/GLEScontext.h"
#include "GLcommon/GLutils.h"

#include <cassert>


TextureData::TextureData(android::base::Stream* stream) : ObjectData(stream) {
    // The current TextureData structure is wrong when dealing with mipmaps.
    target = stream->getBe32();
    width = stream->getBe32();
    height = stream->getBe32();
    depth = stream->getBe32();
    border = stream->getBe32();
    internalFormat = stream->getBe32();
    format = stream->getBe32();
    type = stream->getBe32();
    sourceEGLImage = stream->getBe32();
    hasStorage = stream->getByte();
    wasBound = stream->getByte();
    requiresAutoMipmap = stream->getByte();
    compressed = stream->getByte();
    compressedFormat = stream->getBe32();
    stream->read(crop_rect, sizeof(crop_rect));
    globalName = stream->getBe32();
    loadCollection(stream, &m_texParam,
            [](android::base::Stream* stream) {
                GLenum item = stream->getBe32();
                GLint val = stream->getBe32();
                return std::make_pair(item, val);
    });
}

void TextureData::onSave(android::base::Stream* stream) const {
    ObjectData::onSave(stream);
    // The current TextureData structure is wrong when dealing with mipmaps.
    stream->putBe32(target);
    stream->putBe32(width);
    stream->putBe32(height);
    stream->putBe32(depth);
    stream->putBe32(border);
    stream->putBe32(internalFormat);
    stream->putBe32(format);
    stream->putBe32(type);
    stream->putBe32(sourceEGLImage);
    stream->putByte(hasStorage);
    stream->putByte(wasBound);
    stream->putByte(requiresAutoMipmap);
    stream->putByte(compressed);
    stream->putBe32(compressedFormat);
    stream->write(crop_rect, sizeof(crop_rect));
    stream->putBe32(globalName);
    saveCollection(stream, m_texParam, [](android::base::Stream* stream,
                const std::pair<const GLenum, GLint>& texParam) {
                    stream->putBe32(texParam.first);
                    stream->putBe32(texParam.second);
    });
}

void TextureData::restore(ObjectLocalName localName,
            getGlobalName_t getGlobalName) {
}

void TextureData::setTexParam(GLenum pname, GLint param) {
    m_texParam[pname] = param;
}
