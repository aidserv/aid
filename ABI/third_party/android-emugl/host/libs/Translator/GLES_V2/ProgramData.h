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
#ifndef PROGRAM_DATA_H
#define PROGRAM_DATA_H

#include "ShaderParser.h"

#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>

struct GLUniformDesc {
    GLUniformDesc() = default;
    GLUniformDesc(const char* name, GLint location, GLsizei count, GLboolean transpose,
            GLenum type, GLsizei size, unsigned char* val);
    GLUniformDesc(android::base::Stream* stream);
    GLUniformDesc(GLUniformDesc&&) = default;
    GLUniformDesc& operator=(GLUniformDesc&&) = default;

    GLsizei mCount = 0;
    GLboolean mTranspose = GL_FALSE;
    GLenum mType = (GLenum)0;
    std::vector<unsigned char> mVal;

    std::string mName = {};

    void onSave(android::base::Stream* stream) const;
};

struct AttachedShader {
    GLuint localName = 0;
    ShaderParser* shader = nullptr;
    // linkedSource is only updated when glLinkProgram
    // This is the "real" source the hardware is using for the compiled program.
    std::string linkedSource = {};
};

class ProgramData:public ObjectData{
public:
    enum ShaderType {
        VERTEX = 0,
        FRAGMENT,
        COMPUTE,
        NUM_SHADER_TYPE
    };
    ProgramData();
    ProgramData(android::base::Stream* stream);
    virtual void onSave(android::base::Stream* stream) const override;
    virtual void postLoad(getObjDataPtr_t getObjDataPtr) override;
    // restore() in ProgramData must be executed after shaders
    virtual void restore(ObjectLocalName localName,
           getGlobalName_t getGlobalName) override;

    GLuint getAttachedVertexShader() const;
    GLuint getAttachedFragmentShader() const;
    GLuint getAttachedComputeShader() const;
    GLuint getAttachedShader(GLenum type) const;

    bool attachShader(GLuint shader, ShaderParser* shaderData, GLenum type);
    bool isAttached(GLuint shader) const;
    bool detachShader(GLuint shader);
    void bindAttribLocation(const std::string& var, GLuint loc);
    void linkedAttribLocation(const std::string& var, GLuint loc);

    void appendValidationErrMsg(std::ostringstream& ss);
    bool validateLink(ShaderParser* frag, ShaderParser* vert);
    void setLinkStatus(GLint status);
    GLint getLinkStatus() const;

    void setErrInfoLog();
    void setInfoLog(const GLchar *log);
    const GLchar* getInfoLog() const;

    bool isInUse() const { return IsInUse; }
    void setInUse(bool inUse) { IsInUse = inUse; }

    bool getDeleteStatus() const { return DeleteStatus; }
    void setDeleteStatus(bool status) { DeleteStatus = status; }

    // boundAttribLocs stores the attribute locations assigned by
    // glBindAttribLocation.
    // It will take effect after glLinkProgram.
    std::unordered_map<std::string, GLuint> boundAttribLocs;
    virtual GenNameInfo getGenNameInfo() const override;
    void addProgramName(GLuint name) { ProgramName = name; }
private:
    // linkedAttribLocs stores the attribute locations the guest might
    // know about. It includes all boundAttribLocs before the previous
    // glLinkProgram and all attribute locations retrieved by glGetAttribLocation
    std::unordered_map<std::string, GLuint> linkedAttribLocs;
    std::unordered_map<GLuint, GLUniformDesc> uniforms;
    AttachedShader attachedShaders[NUM_SHADER_TYPE] = {};
    std::string validationInfoLog;
    std::unique_ptr<const GLchar[]> infoLog;
    GLint  LinkStatus;
    bool    IsInUse;
    bool    DeleteStatus;
    GLuint  ProgramName;
    bool needRestore = false;
    std::unordered_map<GLuint, GLuint> mUniformBlockBinding;
    std::vector<std::string> mTransformFeedbacks;
    GLenum mTransformFeedbackBufferMode = 0;
};
#endif
