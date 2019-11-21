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

#include "ShaderParser.h"

#include <memory>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

ShaderParser::ShaderParser(GLenum type) : ObjectData(SHADER_DATA), m_type(type) {}

GenNameInfo ShaderParser::getGenNameInfo() const {
    switch (m_type) {
        case GL_VERTEX_SHADER:
            return GenNameInfo(ShaderProgramType::VERTEX_SHADER);
        case GL_FRAGMENT_SHADER:
            return GenNameInfo(ShaderProgramType::FRAGMENT_SHADER);
        case GL_COMPUTE_SHADER:
            return GenNameInfo(ShaderProgramType::COMPUTE_SHADER);
        default:
            assert(0);
            return GenNameInfo(NamedObjectType::SHADER_OR_PROGRAM);
    }
}

ShaderParser::ShaderParser(android::base::Stream* stream) : ObjectData(stream) {
    m_originalSrc = stream->getString();
    m_src = stream->getString();
    m_parsedSrc = stream->getString();
    m_parsedLines = (GLchar*)m_parsedSrc.c_str();
    m_compiledSrc = stream->getString();
    m_infoLog = stream->getString();
    size_t programSize = stream->getBe32();
    for (size_t program = 0; program < programSize; program++) {
        m_programs.insert(stream->getBe32());
    }
    m_type = stream->getBe32();
    m_compileStatus = stream->getByte();
    m_deleteStatus = stream->getByte();
    m_valid = stream->getByte();
}

void ShaderParser::onSave(android::base::Stream* stream) const {
    // The first byte is used to distinguish between program and shader object.
    // It will be loaded outside of this class
    stream->putByte(LOAD_SHADER);
    ObjectData::onSave(stream);
    stream->putString(m_originalSrc);
    stream->putString(m_src);
    stream->putString(m_parsedSrc);
    // do not snapshot m_parsedLines
    stream->putString(m_compiledSrc);
    stream->putString(m_infoLog);
    stream->putBe32(m_programs.size());
    for (const auto& program : m_programs) {
        stream->putBe32(program);
    }
    stream->putBe32(m_type);
    stream->putByte(m_compileStatus);
    stream->putByte(m_deleteStatus);
    stream->putByte(m_valid);
}

void ShaderParser::restore(ObjectLocalName localName,
           getGlobalName_t getGlobalName) {
    if (m_parsedSrc.empty()) return;
    int globalName = getGlobalName(NamedObjectType::SHADER_OR_PROGRAM,
            localName);
    GLEScontext::dispatcher().glShaderSource(globalName, 1, parsedLines(), NULL);
    if (m_compileStatus)
        GLEScontext::dispatcher().glCompileShader(globalName);
}

static const char kAlwaysInvalidShader[] =
    "When shader translation fails, passing an empty string to underlying GL "
    "may result in GL_COMPILE_STATUS == GL_TRUE and letting us link an "
    "invalid shader program, due to tripping the underlying GL's separable "
    "shader program capability."
    "This shader is meant to make the underlying GL agree that the shader is"
    "indeed invalid when translation fails.";

void ShaderParser::convertESSLToGLSL(int esslVersion) {
    std::string infolog;
    std::string parsedSource;
    m_valid =
        ANGLEShaderParser::translate(
            esslVersion, m_originalSrc.c_str(), m_type,
            &infolog, &parsedSource, &m_shaderLinkInfo);

    if (!m_valid) {
        m_infoLog = static_cast<const GLchar*>(infolog.c_str());
        m_parsedSrc = kAlwaysInvalidShader;
    } else {
        m_parsedSrc = parsedSource;
    }
}

void ShaderParser::setSrc(int esslVersion, GLsizei count, const GLchar* const* strings, const GLint* length){
    m_src.clear();
    for(int i = 0;i<count;i++){
        const size_t strLen =
                (length && length[i] >= 0) ? length[i] : strlen(strings[i]);
        m_src.append(strings[i], strings[i] + strLen);
    }
    // Store original source as some 'parsing' functions actually modify m_src.
    // Note: assign() call is a workaround for the reference-counting
    //  std::string in GCC's STL - we need a deep copy here.
    m_originalSrc.assign(m_src.c_str(), m_src.size());

    convertESSLToGLSL(esslVersion);
}

const GLchar** ShaderParser::parsedLines() {
      m_parsedLines = (GLchar*)m_parsedSrc.c_str();
      return const_cast<const GLchar**> (&m_parsedLines);
}

void ShaderParser::clear() {
    m_parsedLines = nullptr;
    std::string().swap(m_parsedSrc);
    std::string().swap(m_src);
}

const std::string& ShaderParser::getOriginalSrc() const {
    return m_originalSrc;
}

GLenum ShaderParser::getType() {
    return m_type;
}

void ShaderParser::setInfoLog(GLchar* infoLog) {
    assert(infoLog);
    std::unique_ptr<GLchar[]> infoLogDeleter(infoLog);
    m_infoLog.assign(infoLog);
}

bool ShaderParser::validShader() const {
    return m_valid;
}

static const GLchar glsles_invalid[] =
    { "ERROR: Valid GLSL but not GLSL ES" };

void ShaderParser::setInvalidInfoLog() {
    m_infoLog = glsles_invalid;
}

const GLchar* ShaderParser::getInfoLog() const {
    return m_infoLog.c_str();
}

void ShaderParser::setCompileStatus(bool val) {
    m_compileStatus = val;
    m_compiledSrc = m_parsedSrc.c_str();
}
