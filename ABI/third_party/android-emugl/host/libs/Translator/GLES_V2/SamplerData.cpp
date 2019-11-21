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

#include "SamplerData.h"

#include "android/base/files/StreamSerializing.h"
#include "GLcommon/GLEScontext.h"

SamplerData::SamplerData(android::base::Stream* stream)
    : ObjectData(stream) {
    if (stream) {
        android::base::loadCollection(stream, &mParamis,
                [](android::base::Stream* stream) {
                    GLuint idx = stream->getBe32();
                    GLuint val = stream->getBe32();
                    return std::make_pair(idx, val);
                });
        android::base::loadCollection(stream, &mParamfs,
                [](android::base::Stream* stream) {
                    GLuint idx = stream->getBe32();
                    GLfloat val = stream->getFloat();
                    return std::make_pair(idx, val);
                });
    }
}

void SamplerData::onSave(android::base::Stream* stream) const {
    ObjectData::onSave(stream);
    android::base::saveCollection(stream, mParamis,
            [](android::base::Stream* stream,
                const std::pair<const GLenum, GLuint>& item) {
                stream->putBe32(item.first);
                stream->putBe32(item.second);
            });
    android::base::saveCollection(stream, mParamfs,
            [](android::base::Stream* stream,
                const std::pair<const GLenum, GLfloat>& item) {
                stream->putBe32(item.first);
                stream->putFloat(item.second);
            });
}

void SamplerData::restore(ObjectLocalName localName,
        getGlobalName_t getGlobalName) {
    int globalName = getGlobalName(NamedObjectType::SAMPLER,
        localName);
    GLDispatch& dispatcher = GLEScontext::dispatcher();
    for (auto& param : mParamis) {
        dispatcher.glSamplerParameteri(globalName, param.first, param.second);
    }
    for (auto& param : mParamfs) {
        dispatcher.glSamplerParameterf(globalName, param.first, param.second);
    }
}

void SamplerData::setParami(GLenum pname, GLint param) {
    mParamis[pname] = param;
}

void SamplerData::setParamf(GLenum pname, GLfloat param) {
    mParamfs[pname] = param;
}
