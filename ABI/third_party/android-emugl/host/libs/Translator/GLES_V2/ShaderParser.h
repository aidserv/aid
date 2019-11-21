/*
* Copyright 2011 The Android Open Source Project
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

#ifndef SHADER_PARSER_H
#define SHADER_PARSER_H

#include "ANGLEShaderParser.h"
#include "GLESv2Context.h"
#include <string>
#include <GLcommon/ShareGroup.h>
#include <unordered_set>

class ShaderParser : public ObjectData {
public:
    ShaderParser(GLenum type = 0);
    ShaderParser(android::base::Stream* stream);
    virtual void onSave(android::base::Stream* stream) const override;
    virtual void restore(ObjectLocalName localName,
           getGlobalName_t getGlobalName) override;
    void           setSrc(int esslVersion,
                          GLsizei count,
                          const GLchar* const* strings,
                          const GLint* length);
    const std::string& getOriginalSrc() const;
    const GLchar** parsedLines();
    void           clear();
    GLenum         getType();

    // Query whether the shader parsed is valid.
    // Don't trust the value if we did not call setSrc
    bool validShader() const;

    const GLchar* getInfoLog() const;
    void setInfoLog(GLchar * infoLog);

    // If validation fails, add proper error messages
    // to the parser's info log, which is treated
    // as the actual info log from guest POV.
    void setInvalidInfoLog();

    void setCompileStatus(bool val);
    bool getCompileStatus() const { return m_compileStatus; }

    void setDeleteStatus(bool val) { m_deleteStatus = val; }
    bool getDeleteStatus() const { return m_deleteStatus; }

    void attachProgram(GLuint program) {m_programs.insert(program);}
    void detachProgram(GLuint program) {m_programs.erase(program);}
    bool hasAttachedPrograms() const {return m_programs.size()>0;}

    const ANGLEShaderParser::ShaderLinkInfo& getShaderLinkInfo() const { return m_shaderLinkInfo; }

    virtual GenNameInfo getGenNameInfo() const override;
    const char* getCompiledSrc() const { return m_compiledSrc.c_str(); }
private:
    void convertESSLToGLSL(int esslVersion);

    std::string m_originalSrc;
    std::string m_src;
    std::string m_parsedSrc;
    GLchar*     m_parsedLines = nullptr;
    std::string m_compiledSrc;
    std::basic_string<GLchar> m_infoLog;
    std::unordered_set<GLuint> m_programs;
    GLenum      m_type = 0;
    bool        m_compileStatus = false;
    bool        m_deleteStatus = false;
    bool        m_valid = true;
    ANGLEShaderParser::ShaderLinkInfo m_shaderLinkInfo;
};
#endif
