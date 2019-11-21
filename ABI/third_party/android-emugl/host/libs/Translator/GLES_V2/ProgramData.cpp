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

#include "ProgramData.h"
#include "OpenglCodecCommon/glUtils.h"

#include "android/base/containers/Lookup.h"
#include "android/base/files/StreamSerializing.h"

#include <GLcommon/ShareGroup.h>
#include <GLES3/gl31.h>

#include <string.h>
#include <unordered_set>

GLUniformDesc::GLUniformDesc(const char* name, GLint location, GLsizei count, GLboolean transpose,
            GLenum type, GLsizei size, unsigned char* val)
        : mCount(count), mTranspose(transpose), mType(type)
        , mVal(val, val + size), mName(name) { }

GLUniformDesc::GLUniformDesc(android::base::Stream* stream) {
    mCount = stream->getBe32();
    mTranspose = stream->getByte();
    mType = stream->getBe32();
    loadBuffer(stream, &mVal);
    mName = stream->getString();
}

void GLUniformDesc::onSave(android::base::Stream* stream) const {
    stream->putBe32(mCount);
    stream->putByte(mTranspose);
    stream->putBe32(mType);
    saveBuffer(stream, mVal);
    stream->putString(mName);
}

static int s_glShaderType2ShaderType(GLenum type) {
    switch (type) {
    case GL_VERTEX_SHADER:
        return ProgramData::VERTEX;
        break;
    case GL_FRAGMENT_SHADER:
        return ProgramData::FRAGMENT;
        break;
    case GL_COMPUTE_SHADER:
        return ProgramData::COMPUTE;
        break;
    default:
        assert(0);
        break;
    }
    return ProgramData::NUM_SHADER_TYPE;
}

ProgramData::ProgramData() :  ObjectData(PROGRAM_DATA),
                              LinkStatus(GL_FALSE),
                              IsInUse(false),
                              DeleteStatus(false) {}

ProgramData::ProgramData(android::base::Stream* stream) :
    ObjectData(stream) {
    auto loadAttribLocs = [](android::base::Stream* stream) {
                std::string attrib = stream->getString();
                GLuint loc = stream->getBe32();
                return std::make_pair(std::move(attrib), loc);
            };
    loadCollection(stream, &boundAttribLocs, loadAttribLocs);
    loadCollection(stream, &linkedAttribLocs, loadAttribLocs);

    loadCollection(stream, &uniforms, [](android::base::Stream* stream) {
       GLuint loc = stream->getBe32();
       GLUniformDesc desc(stream);
       return std::make_pair(loc, std::move(desc));
    });
    loadCollection(stream, &mUniformBlockBinding,
            [](android::base::Stream* stream) {
                GLuint block = stream->getBe32();
                GLuint binding = stream->getBe32();
                return std::make_pair(block, binding);
    });
    int transformFeedbackCount = stream->getBe32();
    mTransformFeedbacks.resize(transformFeedbackCount);
    for (auto& feedback : mTransformFeedbacks) {
        feedback = stream->getString();
    }
    mTransformFeedbackBufferMode = stream->getBe32();

    for (auto& s : attachedShaders) {
        s.localName = stream->getBe32();
        s.linkedSource = stream->getString();
    }
    validationInfoLog = stream->getString();

    std::string infoLogRead = stream->getString();
    size_t infoSize = infoLogRead.length();
    if (infoSize) {
        GLchar* info = new GLchar[infoSize + 1];
        memcpy(info, infoLogRead.c_str(), infoSize + 1);
        infoLog.reset(info);
    }
    LinkStatus = stream->getBe32();
    IsInUse = stream->getByte();
    DeleteStatus = stream->getByte();
    needRestore = true;
}

static void getUniformValue(GLuint pname, const GLchar *name, GLenum type,
                     std::unordered_map<GLuint, GLUniformDesc> &uniformsOnSave) {
    alignas(double) unsigned char val[256];     //Large enought to hold MAT4x4
    GLDispatch& dispatcher = GLEScontext::dispatcher();

    GLint location = dispatcher.glGetUniformLocation(pname, name);
    if (location < 0) {
        return;
    }
    switch(type) {
    case GL_FLOAT:
    case GL_FLOAT_VEC2:
    case GL_FLOAT_VEC3:
    case GL_FLOAT_VEC4:
    case GL_FLOAT_MAT2:
    case GL_FLOAT_MAT3:
    case GL_FLOAT_MAT4:
    case GL_FLOAT_MAT2x3:
    case GL_FLOAT_MAT2x4:
    case GL_FLOAT_MAT3x2:
    case GL_FLOAT_MAT3x4:
    case GL_FLOAT_MAT4x2:
    case GL_FLOAT_MAT4x3:
        dispatcher.glGetUniformfv(pname, location, (GLfloat *)val);
        break;
    case GL_INT:
    case GL_INT_VEC2:
    case GL_INT_VEC3:
    case GL_INT_VEC4:
    case GL_BOOL:
    case GL_BOOL_VEC2:
    case GL_BOOL_VEC3:
    case GL_BOOL_VEC4:
    case GL_SAMPLER_2D:
    case GL_SAMPLER_3D:
    case GL_SAMPLER_CUBE:
    case GL_SAMPLER_2D_SHADOW:
    case GL_SAMPLER_2D_ARRAY:
    case GL_SAMPLER_2D_ARRAY_SHADOW:
    case GL_SAMPLER_CUBE_SHADOW:
    case GL_INT_SAMPLER_2D:
    case GL_INT_SAMPLER_3D:
    case GL_INT_SAMPLER_CUBE:
    case GL_INT_SAMPLER_2D_ARRAY:
    case GL_UNSIGNED_INT_SAMPLER_2D:
    case GL_UNSIGNED_INT_SAMPLER_3D:
    case GL_UNSIGNED_INT_SAMPLER_CUBE:
    case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
        dispatcher.glGetUniformiv(pname, location, (GLint *)val);
        break;
    case GL_UNSIGNED_INT:
    case GL_UNSIGNED_INT_VEC2:
    case GL_UNSIGNED_INT_VEC3:
    case GL_UNSIGNED_INT_VEC4:
        dispatcher.glGetUniformuiv(pname, location, (GLuint *)val);
        break;
    default:
        fprintf(stderr, "ProgramData::gtUniformValue: warning: "
                "unsupported uniform type 0x%x\n", type);
        return;
    }
    GLUniformDesc uniformDesc(name, location, 1, 0, /*transpose*/
        type, glSizeof(type), val);
    uniformsOnSave[location] = std::move(uniformDesc);
}

// Query uniform variables from driver
static std::unordered_map<GLuint, GLUniformDesc> collectUniformInfo(GLuint pname) {
    GLint uniform_count;
    GLint nameLength;
    std::unordered_map<GLuint, GLUniformDesc> uniformsOnSave;
    GLDispatch& dispatcher = GLEScontext::dispatcher();
    dispatcher.glGetProgramiv(pname, GL_ACTIVE_UNIFORM_MAX_LENGTH, &nameLength);
    if (nameLength == 0) {
        // No active uniform variables exist.
        return uniformsOnSave;
    }
    dispatcher.glGetProgramiv(pname, GL_ACTIVE_UNIFORMS, &uniform_count);
    std::string name;
    for (int i = 0; i < uniform_count; i++) {
        GLint size;
        GLenum type;
        GLsizei length;
        name.resize(nameLength);
        dispatcher.glGetActiveUniform(pname, i, nameLength, &length, &size, &type, &name[0]);
        name.resize(length);
        if (size > 1) {
            // Uniform array, drivers may return 'arrayName' or 'arrayName[0]'
            // as the name of the array.
            // Need to append '[arrayIndex]' after 'arrayName' to query the
            // value for each array member.
            std::string baseName;
            if (name.compare(length - 3, 3, "[0]") == 0) {
                baseName = name.substr(0, length - 3);
            } else {
                baseName = name;
            }
            for (int arrayIndex = 0; arrayIndex < size; arrayIndex++) {
                std::ostringstream oss;
                oss << baseName << '[' << arrayIndex << ']';
                getUniformValue(pname, oss.str().c_str(), type, uniformsOnSave);
            }
        }
        else {
            getUniformValue(pname, name.c_str(), type, uniformsOnSave);
        }
    }
    return uniformsOnSave;
}

static std::unordered_map<GLuint, GLuint> collectUniformBlockInfo(GLuint pname) {
    if (gl_dispatch_get_max_version() < GL_DISPATCH_MAX_GLES_VERSION_3_0) {
        return {};
    }
    GLint uniformBlockCount = 0;
    std::unordered_map<GLuint, GLuint> uniformBlocks;
    GLDispatch& dispatcher = GLEScontext::dispatcher();
    dispatcher.glGetProgramiv(pname, GL_ACTIVE_UNIFORM_BLOCKS,
            &uniformBlockCount);
    for (int i = 0; i < uniformBlockCount; i++) {
        GLint binding = 0;
        dispatcher.glGetActiveUniformBlockiv(pname, i, GL_UNIFORM_BLOCK_BINDING,
                &binding);
        uniformBlocks.emplace(i, binding);
    }
    return uniformBlocks;
}

static std::vector<std::string> collectTransformFeedbackInfo(GLuint pname) {
    if (gl_dispatch_get_max_version() < GL_DISPATCH_MAX_GLES_VERSION_3_0) {
        return {};
    }
    GLint transformFeedbackCount = 0;
    GLint transformFeedbakMaxLength = 0;
    GLDispatch& dispatcher = GLEScontext::dispatcher();
    dispatcher.glGetProgramiv(pname, GL_TRANSFORM_FEEDBACK_VARYINGS,
            &transformFeedbackCount);
    dispatcher.glGetProgramiv(pname, GL_TRANSFORM_FEEDBACK_VARYING_MAX_LENGTH,
            &transformFeedbakMaxLength);

    std::vector<std::string> transformFeedbacks(transformFeedbackCount);
    std::unique_ptr<char[]> nameBuffer(new char [transformFeedbakMaxLength]);

    for (int i = 0; i < transformFeedbackCount; i++) {
        GLsizei size;
        GLenum type;

        dispatcher.glGetTransformFeedbackVarying(pname, i,
                transformFeedbakMaxLength, nullptr, &size, &type,
                nameBuffer.get());
        transformFeedbacks[i] = nameBuffer.get();
    }
    return transformFeedbacks;
}

void ProgramData::onSave(android::base::Stream* stream) const {
    // The first byte is used to distinguish between program and shader object.
    // It will be loaded outside of this class
    // TODO: snapshot shader source in program object (in case shaders got
    // deleted while the program is still alive)
    stream->putByte(LOAD_PROGRAM);
    ObjectData::onSave(stream);
    auto saveAttribLocs = [](android::base::Stream* stream,
            const std::pair<std::string, GLuint>& attribLoc) {
                stream->putString(attribLoc.first);
                stream->putBe32(attribLoc.second);
            };
    saveCollection(stream, boundAttribLocs, saveAttribLocs);
    saveCollection(stream, linkedAttribLocs, saveAttribLocs);

    auto saveUniform = [](android::base::Stream* stream,
                const std::pair<const GLuint, GLUniformDesc>& uniform) {
            stream->putBe32(uniform.first);
            uniform.second.onSave(stream);
        };
    auto saveUniformBlock = [](android::base::Stream* stream,
                const std::pair<const GLuint, GLuint>& uniformBlock) {
            stream->putBe32(uniformBlock.first);
            stream->putBe32(uniformBlock.second);
        };
    auto saveTransformFeedbacks = [](android::base::Stream* stream,
                const std::vector<std::string>& transformFeedbacks) {
            stream->putBe32((int)transformFeedbacks.size());
            for (const auto& feedback : transformFeedbacks) {
                stream->putString(feedback);
            }
        };
    if (needRestore) {
        saveCollection(stream, uniforms, saveUniform);
        saveCollection(stream, mUniformBlockBinding, saveUniformBlock);
        saveTransformFeedbacks(stream, mTransformFeedbacks);
        stream->putBe32(mTransformFeedbackBufferMode);
    } else {
        std::unordered_map<GLuint, GLUniformDesc> uniformsOnSave =
                collectUniformInfo(ProgramName);
        std::unordered_map<GLuint, GLuint> uniformBlocks =
                collectUniformBlockInfo(ProgramName);
        std::vector<std::string> transformFeedbacks =
                collectTransformFeedbackInfo(ProgramName);
        if (gl_dispatch_get_max_version() >= GL_DISPATCH_MAX_GLES_VERSION_3_0) {
            GLEScontext::dispatcher().glGetProgramiv(ProgramName,
                    GL_TRANSFORM_FEEDBACK_BUFFER_MODE,
                    (GLint*)&mTransformFeedbackBufferMode);
        }

        saveCollection(stream, uniformsOnSave, saveUniform);
        saveCollection(stream, uniformBlocks, saveUniformBlock);
        saveTransformFeedbacks(stream, transformFeedbacks);
        stream->putBe32(mTransformFeedbackBufferMode);
    }

    for (const auto& s : attachedShaders) {
        stream->putBe32(s.localName);
        stream->putString(s.linkedSource);
    }
    stream->putString(validationInfoLog);
    stream->putString(infoLog.get());
    stream->putBe32(LinkStatus);
    stream->putByte(IsInUse);
    stream->putByte(DeleteStatus);
}

void ProgramData::postLoad(getObjDataPtr_t getObjDataPtr) {
    for (auto& s : attachedShaders) {
        if (s.localName) {
            s.shader = (ShaderParser*)getObjDataPtr(
                    NamedObjectType::SHADER_OR_PROGRAM, s.localName).get();
        }
    }
}

void ProgramData::restore(ObjectLocalName localName,
           getGlobalName_t getGlobalName) {
    int globalName = getGlobalName(NamedObjectType::SHADER_OR_PROGRAM,
            localName);
    assert(globalName);
    ProgramName = globalName;
    GLDispatch& dispatcher = GLEScontext::dispatcher();
    if (LinkStatus) {
        // Really, each program name corresponds to 2 programs:
        // the one that is already linked, and the one that is not yet linked.
        // We need to restore both.
        GLint tmpShaders[NUM_SHADER_TYPE];
        for (int i = 0; i < NUM_SHADER_TYPE; i++) {
            AttachedShader& s = attachedShaders[i];
            if (s.linkedSource.empty()) {
                tmpShaders[i] = 0;
                continue;
            }
            GLenum type = 0;
            switch (i) {
            case VERTEX:
                type = GL_VERTEX_SHADER;
                break;
            case FRAGMENT:
                type = GL_FRAGMENT_SHADER;
                break;
            case COMPUTE:
                type = GL_COMPUTE_SHADER;
                break;
            default:
                assert(0);
            }
            tmpShaders[i] = dispatcher.glCreateShader(type);
            const GLchar* src = (const GLchar *)s.linkedSource.c_str();
            dispatcher.glShaderSource(tmpShaders[i], 1, &src, NULL);
            dispatcher.glCompileShader(tmpShaders[i]);
            dispatcher.glAttachShader(globalName, tmpShaders[i]);
        }
        for (const auto& attribLocs : linkedAttribLocs) {
            dispatcher.glBindAttribLocation(globalName, attribLocs.second,
                    attribLocs.first.c_str());
        }
        if (gl_dispatch_get_max_version() >= GL_DISPATCH_MAX_GLES_VERSION_3_0) {
            std::unique_ptr<const char*> varyings(
                    new const char*[mTransformFeedbacks.size()]);
            for (size_t i = 0; i < mTransformFeedbacks.size(); i++) {
                varyings.get()[i] = mTransformFeedbacks[i].c_str();
            }
            dispatcher.glTransformFeedbackVaryings(globalName,
                    mTransformFeedbacks.size(), varyings.get(),
                    mTransformFeedbackBufferMode);
            mTransformFeedbacks.clear();
        }
        dispatcher.glLinkProgram(globalName);
        dispatcher.glUseProgram(globalName);
        for (const auto& uniformEntry : uniforms) {
            const auto& uniform = uniformEntry.second;
            GLint location = dispatcher.glGetUniformLocation(globalName,
                    uniform.mName.c_str());
            if (location != (GLint)uniformEntry.first) {
                // Location changed after loading from a snapshot.
                // likely a driver bug
                fprintf(stderr,
                        "%llu ERR: uniform location changed (%s: %d->%d)\n",
                        localName, uniform.mName.c_str(),
                        (GLint)uniformEntry.first, location);
                continue;
            }

            switch (uniform.mType) {
                case GL_FLOAT:
                    dispatcher.glUniform1fv(uniformEntry.first, uniform.mCount,
                            (const GLfloat*)uniform.mVal.data());
                    break;
                case GL_FLOAT_VEC2:
                    dispatcher.glUniform2fv(uniformEntry.first, uniform.mCount,
                            (const GLfloat*)uniform.mVal.data());
                    break;
                case GL_FLOAT_VEC3:
                    dispatcher.glUniform3fv(uniformEntry.first, uniform.mCount,
                            (const GLfloat*)uniform.mVal.data());
                    break;
                case GL_FLOAT_VEC4:
                    dispatcher.glUniform4fv(uniformEntry.first, uniform.mCount,
                            (const GLfloat*)uniform.mVal.data());
                    break;
                case GL_BOOL:
                case GL_INT:
                case GL_SAMPLER_2D:
                case GL_SAMPLER_3D:
                case GL_SAMPLER_CUBE:
                case GL_SAMPLER_2D_SHADOW:
                case GL_SAMPLER_2D_ARRAY:
                case GL_SAMPLER_2D_ARRAY_SHADOW:
                case GL_SAMPLER_CUBE_SHADOW:
                case GL_INT_SAMPLER_2D:
                case GL_INT_SAMPLER_3D:
                case GL_INT_SAMPLER_CUBE:
                case GL_INT_SAMPLER_2D_ARRAY:
                case GL_UNSIGNED_INT_SAMPLER_2D:
                case GL_UNSIGNED_INT_SAMPLER_3D:
                case GL_UNSIGNED_INT_SAMPLER_CUBE:
                case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
                    dispatcher.glUniform1iv(uniformEntry.first, uniform.mCount,
                            (const GLint*)uniform.mVal.data());
                    break;
                case GL_BOOL_VEC2:
                case GL_INT_VEC2:
                    dispatcher.glUniform2iv(uniformEntry.first, uniform.mCount,
                            (const GLint*)uniform.mVal.data());
                    break;
                case GL_BOOL_VEC3:
                case GL_INT_VEC3:
                    dispatcher.glUniform3iv(uniformEntry.first, uniform.mCount,
                            (const GLint*)uniform.mVal.data());
                    break;
                case GL_BOOL_VEC4:
                case GL_INT_VEC4:
                    dispatcher.glUniform4iv(uniformEntry.first, uniform.mCount,
                            (const GLint*)uniform.mVal.data());
                    break;
                case GL_UNSIGNED_INT:
                    dispatcher.glUniform1uiv(uniformEntry.first, uniform.mCount,
                            (const GLuint*)uniform.mVal.data());
                    break;
                case GL_UNSIGNED_INT_VEC2:
                    dispatcher.glUniform2uiv(uniformEntry.first, uniform.mCount,
                            (const GLuint*)uniform.mVal.data());
                    break;
                case GL_UNSIGNED_INT_VEC3:
                    dispatcher.glUniform3uiv(uniformEntry.first, uniform.mCount,
                            (const GLuint*)uniform.mVal.data());
                    break;
                case GL_UNSIGNED_INT_VEC4:
                    dispatcher.glUniform4uiv(uniformEntry.first, uniform.mCount,
                            (const GLuint*)uniform.mVal.data());
                    break;
                case GL_FLOAT_MAT2:
                    dispatcher.glUniformMatrix2fv(uniformEntry.first,
                            uniform.mCount, uniform.mTranspose,
                            (const GLfloat*)uniform.mVal.data());
                    break;
                case GL_FLOAT_MAT3:
                    dispatcher.glUniformMatrix3fv(uniformEntry.first,
                            uniform.mCount, uniform.mTranspose,
                            (const GLfloat*)uniform.mVal.data());
                    break;
                case GL_FLOAT_MAT4:
                    dispatcher.glUniformMatrix4fv(uniformEntry.first,
                            uniform.mCount, uniform.mTranspose,
                            (const GLfloat*)uniform.mVal.data());
                    break;
                case GL_FLOAT_MAT2x3:
                    dispatcher.glUniformMatrix2x3fv(uniformEntry.first,
                            uniform.mCount, uniform.mTranspose,
                            (const GLfloat*)uniform.mVal.data());
                    break;
                case GL_FLOAT_MAT2x4:
                    dispatcher.glUniformMatrix2x4fv(uniformEntry.first,
                            uniform.mCount, uniform.mTranspose,
                            (const GLfloat*)uniform.mVal.data());
                    break;
                case GL_FLOAT_MAT3x2:
                    dispatcher.glUniformMatrix3x2fv(uniformEntry.first,
                            uniform.mCount, uniform.mTranspose,
                            (const GLfloat*)uniform.mVal.data());
                    break;
                case GL_FLOAT_MAT3x4:
                    dispatcher.glUniformMatrix3x4fv(uniformEntry.first,
                            uniform.mCount, uniform.mTranspose,
                            (const GLfloat*)uniform.mVal.data());
                    break;
                case GL_FLOAT_MAT4x2:
                    dispatcher.glUniformMatrix4x2fv(uniformEntry.first,
                            uniform.mCount, uniform.mTranspose,
                            (const GLfloat*)uniform.mVal.data());
                    break;
                case GL_FLOAT_MAT4x3:
                    dispatcher.glUniformMatrix4x3fv(uniformEntry.first,
                            uniform.mCount, uniform.mTranspose,
                            (const GLfloat*)uniform.mVal.data());
                    break;
                default:
                    fprintf(stderr, "ProgramData::restore: warning: "
                            "unsupported uniform type 0x%x\n", uniform.mType);
            }
        }
        for (const auto& uniformBlock : mUniformBlockBinding) {
            dispatcher.glUniformBlockBinding(globalName, uniformBlock.first,
                    uniformBlock.second);
        }
        for (auto s : tmpShaders) {
            if (s != 0) {
                dispatcher.glDetachShader(globalName, s);
                dispatcher.glDeleteShader(s);
            }
        }
    }
    uniforms.clear();
    mUniformBlockBinding.clear();
    // We are done with the "linked" program, now we handle the one
    // that is yet to compile
    for (const auto& s : attachedShaders) {
        if (s.localName) {
            int shaderGlobalName = getGlobalName(
                    NamedObjectType::SHADER_OR_PROGRAM, s.localName);
            assert(shaderGlobalName);
            dispatcher.glAttachShader(globalName, shaderGlobalName);
        }
    }
    for (const auto& attribLocs : boundAttribLocs) {
        dispatcher.glBindAttribLocation(globalName, attribLocs.second,
                attribLocs.first.c_str());
    }
    needRestore = false;
}

GenNameInfo ProgramData::getGenNameInfo() const {
    return GenNameInfo(ShaderProgramType::PROGRAM);
}

void ProgramData::setErrInfoLog() {
    size_t bytes = validationInfoLog.length() + 1;
    infoLog.reset(new GLchar[bytes]);
    memcpy((char*)infoLog.get(), &validationInfoLog[0], bytes);
}

void ProgramData::setInfoLog(const GLchar* log) {
    infoLog.reset(log);
}

const GLchar* ProgramData::getInfoLog() const {
    return infoLog.get() ? infoLog.get() : (const GLchar*)"";
}

GLuint ProgramData::getAttachedVertexShader() const {
    return attachedShaders[VERTEX].localName;
}

GLuint ProgramData::getAttachedFragmentShader() const {
    return attachedShaders[FRAGMENT].localName;
}

GLuint ProgramData::getAttachedComputeShader() const {
    return attachedShaders[COMPUTE].localName;
}

GLuint ProgramData::getAttachedShader(GLenum type) const {
    return attachedShaders[s_glShaderType2ShaderType(type)].localName;
}

bool ProgramData::attachShader(GLuint shader, ShaderParser* shaderData,
        GLenum type) {
    AttachedShader& s = attachedShaders[s_glShaderType2ShaderType(type)];
    if (s.localName == 0) {
        s.localName = shader;
        s.shader = shaderData;
        return true;
    }
    return false;
}

bool ProgramData::isAttached(GLuint shader) const {
    for (const auto& s : attachedShaders) {
        if (s.localName == shader) return true;
    }
    return false;
}

bool ProgramData::detachShader(GLuint shader) {
    for (auto& s : attachedShaders) {
        if (s.localName == shader) {
            s.localName = 0;
            s.shader = nullptr;
            return true;
        }
    }
    return false;
}

void ProgramData::bindAttribLocation(const std::string& var, GLuint loc) {
    boundAttribLocs[var] = loc;
}

void ProgramData::linkedAttribLocation(const std::string& var, GLuint loc) {
    linkedAttribLocs[var] = loc;
}

// Link-time validation
void ProgramData::appendValidationErrMsg(std::ostringstream& ss) {
    validationInfoLog += "Error: " + ss.str() + "\n";
}
static bool sCheckUndecl(ProgramData* pData,
                         const ANGLEShaderParser::ShaderLinkInfo& fragLinkInfo,
                         const ANGLEShaderParser::ShaderLinkInfo& vertLinkInfo);
static bool sCheckLimits(ProgramData* pData,
                         const ShBuiltInResources& resources,
                         const ANGLEShaderParser::ShaderLinkInfo& fragLinkInfo,
                         const ANGLEShaderParser::ShaderLinkInfo& vertLinkInfo);
static bool sCheckVariables(ProgramData* pData,
                            const ANGLEShaderParser::ShaderLinkInfo& a,
                            const ANGLEShaderParser::ShaderLinkInfo& b);
bool ProgramData::validateLink(ShaderParser* frag, ShaderParser* vert) {
    const ANGLEShaderParser::ShaderLinkInfo& fragLinkInfo =
        frag->getShaderLinkInfo();
    const ANGLEShaderParser::ShaderLinkInfo& vertLinkInfo =
        vert->getShaderLinkInfo();

    bool res = true;

    res = res && sCheckUndecl(this, fragLinkInfo, vertLinkInfo);
    res = res && sCheckLimits(this, ANGLEShaderParser::kResources,
                              fragLinkInfo, vertLinkInfo);
    res = res && sCheckVariables(this, fragLinkInfo, vertLinkInfo);

    return res;
}

void ProgramData::setLinkStatus(GLint status) {
    LinkStatus = status;
    if (status) {
        for (auto& s : attachedShaders) {
            if (s.localName) {
                assert(s.shader);
                s.linkedSource = s.shader->getCompiledSrc();
            }
        }
        for (const auto &attribLoc : boundAttribLocs) {
            // overwrite
            linkedAttribLocs[attribLoc.first] = attribLoc.second;
        }
    } else {
        for (auto& s : attachedShaders) {
            s.linkedSource.clear();
        }
    }
}

GLint ProgramData::getLinkStatus() const {
    return LinkStatus;
}

static const char kDifferentPrecisionErr[] =
    "specified with different precision in different shaders.";
static const char kDifferentTypeErr[] =
    "specified with different type in different shaders.";
static const char kDifferentLayoutQualifierErr[] =
    "specified with different layout qualifiers in different shaders.";
static const char kExceededMaxVertexAttribs[] =
    "exceeded max vertex attribs.";
static const char kUsedUndeclaredErr[] =
    "used, but not declared.";
static const char kUniformQualifier[] = "uniform";
static const char kVaryingQualifier[] = "varying";
static const char kUnknownQualifier[] = "[unknown qualifier]";
enum ValidationQualifier {
    UNIFORM,
    VARYING,
};

static const char* sQualifierString(ValidationQualifier q) {
    switch (q) {
    case ValidationQualifier::UNIFORM:
        return kUniformQualifier;
    case ValidationQualifier::VARYING:
        return kVaryingQualifier;
    }
    return kUnknownQualifier;
}

static bool sVarCheck(ProgramData* pData,
                      ValidationQualifier qualifier,
                      const sh::ShaderVariable& a,
                      const sh::ShaderVariable& b) {
    bool res = true;

    if (qualifier == ValidationQualifier::UNIFORM &&
        a.precision != b.precision) {
        std::ostringstream err;
        err << sQualifierString(qualifier) << " " << a.name << " ";
        err << kDifferentPrecisionErr;
        pData->appendValidationErrMsg(err);
        res = false;
    }

    if (a.isStruct() != b.isStruct() ||
        a.type != b.type) {
        std::ostringstream err;
        err << sQualifierString(qualifier) << " " << a.name << " ";
        err << kDifferentTypeErr;
        pData->appendValidationErrMsg(err);
        res = false;
    }

    if (a.isStruct()) {
        for (const auto& afield : a.fields) {
            for (const auto& bfield : b.fields) {
                if (afield.name != bfield.name) continue;
                res = res && sVarCheck(pData, qualifier, afield, bfield);
            }
        }
    }

    return res;
}

static bool sInterfaceBlockCheck(ProgramData* pData,
                                 const sh::InterfaceBlock& a,
                                 const sh::InterfaceBlock& b) {
    bool res = true;

    if (a.layout != b.layout ||
        a.isRowMajorLayout != b.isRowMajorLayout) {
        std::ostringstream err;
        err << "interface block " << a.name << " ";
        err << kDifferentLayoutQualifierErr;
        pData->appendValidationErrMsg(err);
        res = false;
    }

    if (a.fields.size() != b.fields.size()) {
        std::ostringstream err;
        err << "interface block " << a.name << " ";
        err << kDifferentTypeErr;
        pData->appendValidationErrMsg(err);
        res = false;
    }

    for (const auto& afield : a.fields) {
        for (const auto& bfield : b.fields) {
            if (afield.name != bfield.name) continue;
            res = res && sVarCheck(pData, ValidationQualifier::VARYING,
                                   afield, bfield);
            if (afield.isRowMajorLayout != bfield.isRowMajorLayout) {
                std::ostringstream err;
                err << "interface block field ";
                err << a.name << "." << afield.name << " ";
                err << kDifferentLayoutQualifierErr;
                pData->appendValidationErrMsg(err);
                res = false;
            }
        }
    }
    return res;
}

static bool sCheckUndecl(
        ProgramData* pData,
        const ANGLEShaderParser::ShaderLinkInfo& fragLinkInfo,
        const ANGLEShaderParser::ShaderLinkInfo& vertLinkInfo) {
    bool res = true;
    for (const auto& felt : fragLinkInfo.varyings) {
        if (felt.isBuiltIn()) continue;

        bool declaredInVertShader = false;
        for (const auto& velt : vertLinkInfo.varyings) {
            if (velt.name == felt.name) {
                declaredInVertShader = true;
                break;
            }
        }

        if (!declaredInVertShader && felt.staticUse) {
            std::ostringstream err;
            err << "varying " << felt.name << " ";
            err << kUsedUndeclaredErr;
            pData->appendValidationErrMsg(err);
            res = false;
        }
    }
    return res;
}

static bool sCheckLimits(
        ProgramData* pData,
        const ShBuiltInResources& resources,
        const ANGLEShaderParser::ShaderLinkInfo& fragShaderLinkInfo,
        const ANGLEShaderParser::ShaderLinkInfo& vertShaderLinkInfo) {
    bool res = true;

    size_t maxAttribs = (size_t)resources.MaxVertexAttribs;

    std::unordered_set<GLuint> explicitlyBound;
    int numImplicitlyBound = 0;
    for (const auto& elt : vertShaderLinkInfo.attributes) {
        if (const auto loc = android::base::find(pData->boundAttribLocs, elt.name)) {
            explicitlyBound.insert(*loc);
        } else {
            numImplicitlyBound++;
        }
    }
    int numExplicitlyBound = explicitlyBound.size();

    if ((int)maxAttribs - numExplicitlyBound - numImplicitlyBound < 0) {
        std::ostringstream err;
        err << kExceededMaxVertexAttribs;
        err << " Wanted (from vertex shader): ";
        err << numExplicitlyBound + numImplicitlyBound << " ";
        err << " Limit: " << maxAttribs << " ";
        pData->appendValidationErrMsg(err);
        res = false;
    }

    return res;
}

static bool sCheckVariables(ProgramData* pData,
                            const ANGLEShaderParser::ShaderLinkInfo& a,
                            const ANGLEShaderParser::ShaderLinkInfo& b) {
    bool res = true;

    for (const auto& aelt : a.uniforms) {
        for (const auto& belt : b.uniforms) {
            if (aelt.name != belt.name) continue;
            res = res && sVarCheck(pData, ValidationQualifier::UNIFORM, aelt, belt);
        }
    }

    for (const auto& aelt : a.varyings) {
        for (const auto& belt : b.varyings) {
            if (aelt.name != belt.name) continue;
            res = res && sVarCheck(pData, ValidationQualifier::VARYING, aelt, belt);
        }
    }

    for (const auto& aelt : a.interfaceBlocks) {
        for (const auto& belt : b.interfaceBlocks) {
            if (aelt.name != belt.name) continue;
            res = res && sInterfaceBlockCheck(pData, aelt, belt);
        }
    }

    return res;
}


