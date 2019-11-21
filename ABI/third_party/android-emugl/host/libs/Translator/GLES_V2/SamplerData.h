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

#pragma once

#include "GLcommon/ObjectData.h"

#include <unordered_map>

class SamplerData : public ObjectData {
public:
    SamplerData() : ObjectData(SAMPLER_DATA) {}
    SamplerData(android::base::Stream* stream);
    virtual void onSave(android::base::Stream* stream) const override;
    virtual void restore(ObjectLocalName localName,
           getGlobalName_t getGlobalName) override;
    void setParami(GLenum pname, GLint param);
    void setParamf(GLenum pname, GLfloat param);
private:
    std::unordered_map<GLenum, GLint> mParamis;
    std::unordered_map<GLenum, GLfloat> mParamfs;
};
