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

#include "GLcommon/NamedObject.h"

#include "emugl/common/mutex.h"
#include "GLcommon/GLEScontext.h"
#include "GLcommon/ObjectNameSpace.h"

NamedObject::NamedObject(GenNameInfo genNameInfo,
                         GlobalNameSpace *globalNameSpace) {
    // We need a global mutex from globalNameSpace
    // And we can't change the mutex to a static variable because it would be
    // passed around different shared libraries.
    m_globalNameSpace = globalNameSpace;
    emugl::Mutex::AutoLock _lock(m_globalNameSpace->m_lock);
    m_type = genNameInfo.m_type;
    if (genNameInfo.m_existingGlobal) {
        fprintf(stderr, "%s: global name %u exists already\n", __func__, genNameInfo.m_existingGlobal);
        m_globalName = genNameInfo.m_existingGlobal;
    } else {
        switch (genNameInfo.m_type) {
            case NamedObjectType::VERTEXBUFFER:
                GLEScontext::dispatcher().glGenBuffers(1,&m_globalName);
                break;
            case NamedObjectType::TEXTURE:
                GLEScontext::dispatcher().glGenTextures(1,&m_globalName);
                break;
            case NamedObjectType::RENDERBUFFER:
                GLEScontext::dispatcher().glGenRenderbuffersEXT(1, &m_globalName);
                break;
            case NamedObjectType::FRAMEBUFFER:
                GLEScontext::dispatcher().glGenFramebuffersEXT(1,&m_globalName);
                break;
            case NamedObjectType::SHADER_OR_PROGRAM:
                switch (genNameInfo.m_shaderProgramType) {
                    case ShaderProgramType::PROGRAM:
                        m_globalName = GLEScontext::dispatcher().glCreateProgram();
                        break;
                    case ShaderProgramType::VERTEX_SHADER:
                        m_globalName = GLEScontext::dispatcher().glCreateShader(
                                GL_VERTEX_SHADER);
                        break;
                    case ShaderProgramType::FRAGMENT_SHADER:
                        m_globalName = GLEScontext::dispatcher().glCreateShader(
                                GL_FRAGMENT_SHADER);
                        break;
                    case ShaderProgramType::COMPUTE_SHADER:
                        m_globalName = GLEScontext::dispatcher().glCreateShader(
                                GL_COMPUTE_SHADER);
                        break;
                }
                break;
            case NamedObjectType::SAMPLER:
                GLEScontext::dispatcher().glGenSamplers(1, &m_globalName);
                break;
            case NamedObjectType::QUERY:
                GLEScontext::dispatcher().glGenQueries(1, &m_globalName);
                break;
            case NamedObjectType::VERTEX_ARRAY_OBJECT:
                GLEScontext::dispatcher().glGenVertexArrays(1, &m_globalName);
                break;
            default:
                m_globalName = 0;
        }
    }
}

NamedObject::~NamedObject() {
    emugl::Mutex::AutoLock _lock(m_globalNameSpace->m_lock);
    assert(GLEScontext::dispatcher().isInitialized());
    switch (m_type) {
    case NamedObjectType::VERTEXBUFFER:
        GLEScontext::dispatcher().glDeleteBuffers(1, &m_globalName);
        break;
    case NamedObjectType::TEXTURE:
        GLEScontext::dispatcher().glDeleteTextures(1, &m_globalName);
        break;
    case NamedObjectType::RENDERBUFFER:
        GLEScontext::dispatcher().glDeleteRenderbuffersEXT(1, &m_globalName);
        break;
    case NamedObjectType::FRAMEBUFFER:
        GLEScontext::dispatcher().glDeleteFramebuffersEXT(1, &m_globalName);
        break;
    case NamedObjectType::SHADER_OR_PROGRAM:
        if (GLEScontext::dispatcher().glIsProgram(m_globalName)) {
            GLEScontext::dispatcher().glDeleteProgram(m_globalName);
        } else {
            GLEScontext::dispatcher().glDeleteShader(m_globalName);
        }
        break;
    case NamedObjectType::SAMPLER:
        GLEScontext::dispatcher().glDeleteSamplers(1, &m_globalName);
        break;
    case NamedObjectType::QUERY:
        GLEScontext::dispatcher().glDeleteQueries(1, &m_globalName);
        break;
    case NamedObjectType::VERTEX_ARRAY_OBJECT:
        GLEScontext::dispatcher().glDeleteVertexArrays(1, &m_globalName);
        break;
    default:
        break;
    }
}

