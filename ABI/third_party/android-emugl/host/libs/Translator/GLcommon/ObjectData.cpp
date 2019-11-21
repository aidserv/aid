/*
* Copyright (C) 2016 The Android Open Source Project
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

#include "GLcommon/ObjectData.h"

NamedObjectType ObjectDataType2NamedObjectType(ObjectDataType objDataType) {
    switch (objDataType) {
        case SHADER_DATA:
        case PROGRAM_DATA:
            return NamedObjectType::SHADER_OR_PROGRAM;
        case TEXTURE_DATA:
            return NamedObjectType::TEXTURE;
        case BUFFER_DATA:
            return NamedObjectType::VERTEXBUFFER;
        case RENDERBUFFER_DATA:
            return NamedObjectType::RENDERBUFFER;
        case FRAMEBUFFER_DATA:
            return NamedObjectType::FRAMEBUFFER;
        case SAMPLER_DATA:
            return NamedObjectType::SAMPLER;
        default:
            return NamedObjectType::NULLTYPE;
    }
}

ObjectData::ObjectData(android::base::Stream* stream) {
    m_dataType = (ObjectDataType)stream->getBe32();
}

void ObjectData::onSave(android::base::Stream* stream) const {
    stream->putBe32(m_dataType);
}

void ObjectData::postLoad(getObjDataPtr_t getObjDataPtr) {
    (void)getObjDataPtr;
}

GenNameInfo ObjectData::getGenNameInfo() const {
    return GenNameInfo(ObjectDataType2NamedObjectType(m_dataType));
}
