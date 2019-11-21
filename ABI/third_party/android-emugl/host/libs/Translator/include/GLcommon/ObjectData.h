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

#pragma once

#include "android/base/files/Stream.h"
#include "emugl/common/smart_ptr.h"
#include "GLcommon/NamedObject.h"

#include <functional>

enum ObjectDataType {
    SHADER_DATA,
    PROGRAM_DATA,
    TEXTURE_DATA,
    BUFFER_DATA,
    RENDERBUFFER_DATA,
    FRAMEBUFFER_DATA,
    SAMPLER_DATA,
    UNDEFINED_DATA,
};

class ObjectData;
typedef emugl::SmartPtr<ObjectData> ObjectDataPtr;

extern NamedObjectType ObjectDataType2NamedObjectType(ObjectDataType objDataType);

class ObjectData
{
public:
    ObjectData(ObjectDataType type = UNDEFINED_DATA): m_dataType(type) {}
    ObjectData(android::base::Stream* stream);
    ObjectDataType getDataType() { return m_dataType; };
    virtual ~ObjectData() = default;
    virtual void onSave(android::base::Stream* stream) const = 0;
    typedef std::function<ObjectDataPtr(NamedObjectType p_type,
            ObjectLocalName p_localName, android::base::Stream* stream)>
                loadObject_t;
    typedef std::function<const ObjectDataPtr(NamedObjectType,
            ObjectLocalName)> getObjDataPtr_t;
    typedef std::function<int(NamedObjectType, ObjectLocalName)>
            getGlobalName_t;
    // postLoad: setup references after loading all ObjectData from snapshot
    // in one share group
    virtual void postLoad(getObjDataPtr_t getObjDataPtr);
    // restore: restore object data back to hardware GPU.
    //          It must be called before restoring hardware context states,
    //          because it messes up context object bindings.
    virtual void restore(ObjectLocalName localName,
            getGlobalName_t getGlobalName) = 0;
    virtual GenNameInfo getGenNameInfo() const;
private:
    ObjectDataType m_dataType;
};

