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

#include "GLESv2Decoder.h"
#include "GLESv2Dispatch.h"

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl31.h>

#include <string>
#include <vector>

#include <string.h>

static inline void* SafePointerFromUInt(GLuint value) {
  return (void*)(uintptr_t)value;
}

GLESv2Decoder::GLESv2Decoder()
{
    m_contextData = NULL;
    m_GL2library = NULL;
    m_snapshot = NULL;
}

GLESv2Decoder::~GLESv2Decoder()
{
}
 
void *GLESv2Decoder::s_getProc(const char *name, void *userData)
{
    GLESv2Decoder *ctx = (GLESv2Decoder *) userData;

    if (ctx == NULL || ctx->m_GL2library == NULL) {
        return NULL;
    }

    void *func = NULL;
#ifdef USE_EGL_GETPROCADDRESS
    func = (void *) eglGetProcAddress(name);
#endif
    if (func == NULL) {
        func = (void *) ctx->m_GL2library->findSymbol(name);
    }
    return func;
}

#define OVERRIDE_DEC(func) func##_dec = s_##func;

int GLESv2Decoder::initGL(get_proc_func_t getProcFunc, void *getProcFuncData)
{
    this->initDispatchByName(getProcFunc, getProcFuncData);
    glVertexAttribPointerWithDataSize =
            (glVertexAttribPointerWithDataSize_server_proc_t)
            getProcFunc("glVertexAttribPointerWithDataSize", getProcFuncData);

    glGetCompressedTextureFormats = s_glGetCompressedTextureFormats;
    glVertexAttribPointerData = s_glVertexAttribPointerData;
    glVertexAttribPointerOffset = s_glVertexAttribPointerOffset;
    glShaderString = s_glShaderString;

    glDrawElementsOffset = s_glDrawElementsOffset;
    glDrawElementsData = s_glDrawElementsData;
    glFinishRoundTrip = s_glFinishRoundTrip;
    glMapBufferRangeAEMU = s_glMapBufferRangeAEMU;
    glUnmapBufferAEMU = s_glUnmapBufferAEMU;
    glFlushMappedBufferRangeAEMU = s_glFlushMappedBufferRangeAEMU;
    glCompressedTexImage2DOffsetAEMU = s_glCompressedTexImage2DOffsetAEMU;
    glCompressedTexSubImage2DOffsetAEMU = s_glCompressedTexSubImage2DOffsetAEMU;
    glTexImage2DOffsetAEMU = s_glTexImage2DOffsetAEMU;
    glTexSubImage2DOffsetAEMU = s_glTexSubImage2DOffsetAEMU;
    glGetUniformIndicesAEMU = s_glGetUniformIndicesAEMU;
    glVertexAttribIPointerDataAEMU = s_glVertexAttribIPointerDataAEMU;
    glVertexAttribIPointerOffsetAEMU = s_glVertexAttribIPointerOffsetAEMU;
    glTransformFeedbackVaryingsAEMU = s_glTransformFeedbackVaryingsAEMU;
    glTexImage3DOffsetAEMU = s_glTexImage3DOffsetAEMU;
    glTexSubImage3DOffsetAEMU = s_glTexSubImage3DOffsetAEMU;
    glCompressedTexImage3DOffsetAEMU = s_glCompressedTexImage3DOffsetAEMU;
    glCompressedTexSubImage3DOffsetAEMU = s_glCompressedTexSubImage3DOffsetAEMU;
    glDrawElementsInstancedOffsetAEMU = s_glDrawElementsInstancedOffsetAEMU;
    glDrawElementsInstancedDataAEMU = s_glDrawElementsInstancedDataAEMU;
    glReadPixelsOffsetAEMU = s_glReadPixelsOffsetAEMU;

    glCreateShaderProgramvAEMU = s_glCreateShaderProgramvAEMU;

    glDrawArraysIndirectDataAEMU = s_glDrawArraysIndirectDataAEMU;
    glDrawArraysIndirectOffsetAEMU = s_glDrawArraysIndirectOffsetAEMU;

    glDrawElementsIndirectDataAEMU = s_glDrawElementsIndirectDataAEMU;
    glDrawElementsIndirectOffsetAEMU = s_glDrawElementsIndirectOffsetAEMU;

    glFenceSyncAEMU = s_glFenceSyncAEMU;
    glClientWaitSyncAEMU = s_glClientWaitSyncAEMU;
    glWaitSyncAEMU = s_glWaitSyncAEMU;
    glIsSyncAEMU = s_glIsSyncAEMU;
    glGetSyncivAEMU = s_glGetSyncivAEMU;
    glDeleteSyncAEMU = s_glDeleteSyncAEMU;

    OVERRIDE_DEC(glCreateShader)
    OVERRIDE_DEC(glCreateProgram)

    OVERRIDE_DEC(glGenBuffers)

    OVERRIDE_DEC(glGenFramebuffers)
    OVERRIDE_DEC(glGenRenderbuffers)
    OVERRIDE_DEC(glGenTextures)

    OVERRIDE_DEC(glGenVertexArraysOES)
    OVERRIDE_DEC(glGenVertexArrays)

    OVERRIDE_DEC(glGenTransformFeedbacks)
    OVERRIDE_DEC(glGenSamplers)
    OVERRIDE_DEC(glGenQueries)
    OVERRIDE_DEC(glGenProgramPipelines)

    OVERRIDE_DEC(glDeleteShader)
    OVERRIDE_DEC(glDeleteProgram)

    OVERRIDE_DEC(glDeleteBuffers)
    OVERRIDE_DEC(glDeleteFramebuffers)
    OVERRIDE_DEC(glDeleteRenderbuffers)
    OVERRIDE_DEC(glDeleteTextures)

    OVERRIDE_DEC(glDeleteVertexArraysOES)
    OVERRIDE_DEC(glDeleteVertexArrays)

    OVERRIDE_DEC(glDeleteTransformFeedbacks)
    OVERRIDE_DEC(glDeleteSamplers)
    OVERRIDE_DEC(glDeleteQueries)
    OVERRIDE_DEC(glDeleteProgramPipelines)

    // Shaders and programs
    OVERRIDE_DEC(glCompileShader)
    OVERRIDE_DEC(glAttachShader)
    OVERRIDE_DEC(glDetachShader)
    OVERRIDE_DEC(glLinkProgram)
    OVERRIDE_DEC(glUseProgram)
    OVERRIDE_DEC(glValidateProgram)
    OVERRIDE_DEC(glIsShader)
    OVERRIDE_DEC(glIsProgram)
    OVERRIDE_DEC(glGetShaderiv)
    OVERRIDE_DEC(glGetProgramiv)
    OVERRIDE_DEC(glGetShaderInfoLog)
    OVERRIDE_DEC(glGetProgramInfoLog)
    OVERRIDE_DEC(glGetShaderSource)
    OVERRIDE_DEC(glBindAttribLocation)
    OVERRIDE_DEC(glGetActiveAttrib)
    OVERRIDE_DEC(glGetActiveUniform)
    OVERRIDE_DEC(glGetAttachedShaders)
    OVERRIDE_DEC(glGetAttribLocation)
    OVERRIDE_DEC(glGetUniformfv)
    OVERRIDE_DEC(glGetUniformiv)
    OVERRIDE_DEC(glGetUniformLocation)
    OVERRIDE_DEC(glGetProgramBinaryOES)
    OVERRIDE_DEC(glProgramBinaryOES)
    OVERRIDE_DEC(glUniformBlockBinding)
    OVERRIDE_DEC(glGetUniformBlockIndex)
    OVERRIDE_DEC(glGetActiveUniformBlockiv)
    OVERRIDE_DEC(glGetActiveUniformBlockName)
    OVERRIDE_DEC(glGetUniformuiv)
    OVERRIDE_DEC(glGetActiveUniformsiv)
    OVERRIDE_DEC(glTransformFeedbackVaryings)
    OVERRIDE_DEC(glGetTransformFeedbackVarying)
    OVERRIDE_DEC(glProgramParameteri)
    OVERRIDE_DEC(glProgramBinary)
    OVERRIDE_DEC(glGetProgramBinary)
    OVERRIDE_DEC(glGetFragDataLocation)
    OVERRIDE_DEC(glUseProgramStages)
    OVERRIDE_DEC(glActiveShaderProgram)
    OVERRIDE_DEC(glProgramUniform1f)
    OVERRIDE_DEC(glProgramUniform2f)
    OVERRIDE_DEC(glProgramUniform3f)
    OVERRIDE_DEC(glProgramUniform4f)
    OVERRIDE_DEC(glProgramUniform1i)
    OVERRIDE_DEC(glProgramUniform2i)
    OVERRIDE_DEC(glProgramUniform3i)
    OVERRIDE_DEC(glProgramUniform4i)
    OVERRIDE_DEC(glProgramUniform1ui)
    OVERRIDE_DEC(glProgramUniform2ui)
    OVERRIDE_DEC(glProgramUniform3ui)
    OVERRIDE_DEC(glProgramUniform4ui)
    OVERRIDE_DEC(glProgramUniform1fv)
    OVERRIDE_DEC(glProgramUniform2fv)
    OVERRIDE_DEC(glProgramUniform3fv)
    OVERRIDE_DEC(glProgramUniform4fv)
    OVERRIDE_DEC(glProgramUniform1iv)
    OVERRIDE_DEC(glProgramUniform2iv)
    OVERRIDE_DEC(glProgramUniform3iv)
    OVERRIDE_DEC(glProgramUniform4iv)
    OVERRIDE_DEC(glProgramUniform1uiv)
    OVERRIDE_DEC(glProgramUniform2uiv)
    OVERRIDE_DEC(glProgramUniform3uiv)
    OVERRIDE_DEC(glProgramUniform4uiv)
    OVERRIDE_DEC(glProgramUniformMatrix2fv)
    OVERRIDE_DEC(glProgramUniformMatrix3fv)
    OVERRIDE_DEC(glProgramUniformMatrix4fv)
    OVERRIDE_DEC(glProgramUniformMatrix2x3fv)
    OVERRIDE_DEC(glProgramUniformMatrix3x2fv)
    OVERRIDE_DEC(glProgramUniformMatrix2x4fv)
    OVERRIDE_DEC(glProgramUniformMatrix4x2fv)
    OVERRIDE_DEC(glProgramUniformMatrix3x4fv)
    OVERRIDE_DEC(glProgramUniformMatrix4x3fv)
    OVERRIDE_DEC(glGetProgramInterfaceiv)
    OVERRIDE_DEC(glGetProgramResourceiv)
    OVERRIDE_DEC(glGetProgramResourceIndex)
    OVERRIDE_DEC(glGetProgramResourceLocation)
    OVERRIDE_DEC(glGetProgramResourceName)

    return 0;

}

int GLESv2Decoder::s_glFinishRoundTrip(void *self)
{
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glFinish();
    return 0;
}

void GLESv2Decoder::s_glGetCompressedTextureFormats(void *self, int count, GLint *formats)
{
    GLESv2Decoder *ctx = (GLESv2Decoder *) self;

    int nFormats;
    ctx->glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &nFormats);
    if (nFormats > count) {
        fprintf(stderr, "%s: GetCompressedTextureFormats: The requested number of formats does not match the number that is reported by OpenGL\n", __FUNCTION__);
    } else {
        ctx->glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, formats);
    }
}

void GLESv2Decoder::s_glVertexAttribPointerData(void *self, GLuint indx, GLint size, GLenum type,
                                             GLboolean normalized, GLsizei stride,  void * data, GLuint datalen)
{
    GLESv2Decoder *ctx = (GLESv2Decoder *) self;
    
    if (ctx->m_contextData != NULL) {
        ctx->m_contextData->storePointerData(indx, data, datalen);
        // note - the stride of the data is always zero when it comes out of the codec.
        // See gl2.attrib for the packing function call.
        // 0){//
        if ((void*)ctx->glVertexAttribPointerWithDataSize != gles2_unimplemented) {
            ctx->glVertexAttribPointerWithDataSize(indx, size, type, normalized,
                    0, ctx->m_contextData->pointerData(indx), datalen);
        } else {
            ctx->glVertexAttribPointer(indx, size, type, normalized, 0,
                    ctx->m_contextData->pointerData(indx));
        }
    }
}

void GLESv2Decoder::s_glVertexAttribPointerOffset(void *self, GLuint indx, GLint size, GLenum type,
                                               GLboolean normalized, GLsizei stride,  GLuint data)
{
    GLESv2Decoder *ctx = (GLESv2Decoder *) self;
    ctx->glVertexAttribPointer(indx, size, type, normalized, stride, SafePointerFromUInt(data));
}


void GLESv2Decoder::s_glDrawElementsData(void *self, GLenum mode, GLsizei count, GLenum type, void * data, GLuint datalen)
{
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glDrawElements(mode, count, type, data);
}


void GLESv2Decoder::s_glDrawElementsOffset(void *self, GLenum mode, GLsizei count, GLenum type, GLuint offset)
{
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glDrawElements(mode, count, type, SafePointerFromUInt(offset));
}

void GLESv2Decoder::s_glMapBufferRangeAEMU(void* self, GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access, void* mapped)
{
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    if ((access & GL_MAP_READ_BIT) ||
        ((access & GL_MAP_WRITE_BIT) &&
         (!(access & GL_MAP_INVALIDATE_RANGE_BIT) &&
          !(access & GL_MAP_INVALIDATE_BUFFER_BIT)))) {
        void* gpu_ptr = ctx->glMapBufferRange(target, offset, length, access);
        memcpy(mapped, gpu_ptr, length);
        ctx->glUnmapBuffer(target);
    } else {
        // if writing while not wanting to preserve previous contents,
        // let |mapped| stay as garbage.
    }
}

void GLESv2Decoder::s_glUnmapBufferAEMU(void* self, GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access, void* guest_buffer, GLboolean* out_res)
{
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    *out_res = GL_TRUE;

    if (access & GL_MAP_WRITE_BIT) {
        if (!guest_buffer) fprintf(stderr, "%s: error: wanted to write to a mapped buffer with NULL!\n", __FUNCTION__);
        void* gpu_ptr = ctx->glMapBufferRange(target, offset, length, access);
        if (!gpu_ptr) fprintf(stderr, "%s: could not get host gpu pointer!\n", __FUNCTION__);
        memcpy(gpu_ptr, guest_buffer, length);
        *out_res = ctx->glUnmapBuffer(target);
    }
}

void GLESv2Decoder::s_glFlushMappedBufferRangeAEMU(void* self, GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access, void* guest_buffer) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    if (!guest_buffer) fprintf(stderr, "%s: error: wanted to write to a mapped buffer with NULL!\n", __FUNCTION__);
    void* gpu_ptr = ctx->glMapBufferRange(target, offset, length, access);
    memcpy(gpu_ptr, guest_buffer, length);
    if (!gpu_ptr) fprintf(stderr, "%s: could not get host gpu pointer!\n", __FUNCTION__);
    // |offset| was the absolute offset into the mapping, so just flush offset 0.
    ctx->glFlushMappedBufferRange(target, 0, length);
    ctx->glUnmapBuffer(target);
}

void GLESv2Decoder::s_glCompressedTexImage2DOffsetAEMU(void* self, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, GLuint offset) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
	ctx->glCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, SafePointerFromUInt(offset));
}
void GLESv2Decoder::s_glCompressedTexSubImage2DOffsetAEMU(void* self, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, GLuint offset) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
	ctx->glCompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, SafePointerFromUInt(offset));
}
void GLESv2Decoder::s_glTexImage2DOffsetAEMU(void* self, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, GLuint offset) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
	ctx->glTexImage2D(target, level, internalformat, width, height, border, format, type, SafePointerFromUInt(offset));
}
void GLESv2Decoder::s_glTexSubImage2DOffsetAEMU(void* self, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, GLuint offset) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
	ctx->glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, SafePointerFromUInt(offset));
}

static const char* const kNameDelimiter = ";";

static std::vector<std::string> sUnpackVarNames(GLsizei count, const char* packedNames) {
    std::vector<std::string> unpacked;
    GLsizei current = 0;

    while (current < count) {
        const char* delimPos = strstr(packedNames, kNameDelimiter);
        size_t nameLen = delimPos - packedNames;
        std::string next;
        next.resize(nameLen);
        memcpy(&next[0], packedNames, nameLen);
        unpacked.push_back(next);
        packedNames = delimPos + 1;
        current++;
    }

    return unpacked;
}

void GLESv2Decoder::s_glGetUniformIndicesAEMU(void* self, GLuint program, GLsizei uniformCount, const GLchar* packedNames, GLsizei packedLen, GLuint* uniformIndices) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;

    std::vector<std::string> unpacked = sUnpackVarNames(uniformCount, packedNames);

    GLchar** unpackedArray = new GLchar*[unpacked.size()];
    GLsizei i = 0;
    #if 1
    for (auto& elt : unpacked) {
        unpackedArray[i] = &elt[0];
        i++;
    }
    #else
    for (i = 0;i< (GLsizei)unpacked.size();i++) {
        unpackedArray[i] = (GLchar*)&unpacked[i][0];
    }
    #endif

    ctx->glGetUniformIndices(program, uniformCount, (const GLchar**)unpackedArray, uniformIndices);

    delete [] unpackedArray;
}

void GLESv2Decoder::s_glVertexAttribIPointerDataAEMU(void *self, GLuint indx, GLint size, GLenum type, GLsizei stride, void * data, GLuint datalen)
{
    GLESv2Decoder *ctx = (GLESv2Decoder *) self;
    if (ctx->m_contextData != NULL) {
        ctx->m_contextData->storePointerData(indx, data, datalen);
        // note - the stride of the data is always zero when it comes out of the codec.
        // See gl2.attrib for the packing function call.
        ctx->glVertexAttribIPointer(indx, size, type, 0, ctx->m_contextData->pointerData(indx));
    }
}

void GLESv2Decoder::s_glVertexAttribIPointerOffsetAEMU(void *self, GLuint indx, GLint size, GLenum type, GLsizei stride, GLuint data)
{
    GLESv2Decoder *ctx = (GLESv2Decoder *) self;
    ctx->glVertexAttribIPointer(indx, size, type, stride, SafePointerFromUInt(data));
}

void GLESv2Decoder::s_glTransformFeedbackVaryingsAEMU(void* self, GLuint program, GLsizei count, const char* packedVaryings, GLuint packedVaryingsLen, GLenum bufferMode) {

    GLESv2Decoder *ctx = (GLESv2Decoder *) self;

    std::vector<std::string> unpacked = sUnpackVarNames(count, packedVaryings);

    char** unpackedArray = new char*[unpacked.size()];
    GLsizei i = 0;
    #if 1
    for (auto& elt : unpacked) {
        unpackedArray[i] = &elt[0];
        i++;
    }
    #else
    for (i = 0;i< (GLsizei)unpacked.size();i++) {
        unpackedArray[i] = (GLchar*)&unpacked[i][0];
    }
    #endif

    ctx->glTransformFeedbackVaryings(program, count, (const char**)unpackedArray, bufferMode);

    delete [] unpackedArray;
}

void GLESv2Decoder::s_glTexImage3DOffsetAEMU(void* self, GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, GLuint offset) {
    GLESv2Decoder *ctx = (GLESv2Decoder *) self;
	ctx->glTexImage3D(target, level, internalFormat, width, height, depth, border, format, type, SafePointerFromUInt(offset));
}
void GLESv2Decoder::s_glTexSubImage3DOffsetAEMU(void* self, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, GLuint offset) {
    GLESv2Decoder *ctx = (GLESv2Decoder *) self;
	ctx->glTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, SafePointerFromUInt(offset));
}
void GLESv2Decoder::s_glCompressedTexImage3DOffsetAEMU(void* self, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, GLuint offset) {
    GLESv2Decoder *ctx = (GLESv2Decoder *) self;
	ctx->glCompressedTexImage3D(target, level, internalformat, width, height, depth, border, imageSize, SafePointerFromUInt(offset));
}
void GLESv2Decoder::s_glCompressedTexSubImage3DOffsetAEMU(void* self, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, GLuint offset) {
    GLESv2Decoder *ctx = (GLESv2Decoder *) self;
	ctx->glCompressedTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, SafePointerFromUInt(offset));
}

void GLESv2Decoder::s_glDrawElementsInstancedOffsetAEMU(void* self, GLenum mode, GLsizei count, GLenum type, GLuint offset, GLsizei primcount) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glDrawElementsInstanced(mode, count, type, SafePointerFromUInt(offset), primcount);
}

void GLESv2Decoder::s_glDrawElementsInstancedDataAEMU(void* self, GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei primcount, GLsizei datalen) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glDrawElementsInstanced(mode, count, type, indices, primcount);
}

void GLESv2Decoder::s_glReadPixelsOffsetAEMU(void* self, GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLuint offset) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glReadPixels(x, y, width, height, format, type, SafePointerFromUInt(offset));
}

GLuint GLESv2Decoder::s_glCreateShaderProgramvAEMU(void* self, GLenum type, GLsizei count, const char* packedStrings, GLuint packedLen) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    return ctx->glCreateShaderProgramv(type, 1, &packedStrings);
    // TODO: Snapshot names
}

void GLESv2Decoder::s_glDrawArraysIndirectDataAEMU(void* self, GLenum mode, const void* indirect, GLuint datalen) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glDrawArraysIndirect(mode, indirect);
}

void GLESv2Decoder::s_glDrawArraysIndirectOffsetAEMU(void* self, GLenum mode, GLuint offset) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glDrawArraysIndirect(mode, SafePointerFromUInt(offset));
}

void GLESv2Decoder::s_glDrawElementsIndirectDataAEMU(void* self, GLenum mode, GLenum type, const void* indirect, GLuint datalen) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glDrawElementsIndirect(mode, type, indirect);
}

void GLESv2Decoder::s_glDrawElementsIndirectOffsetAEMU(void* self, GLenum mode, GLenum type, GLuint offset) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glDrawElementsIndirect(mode, type, SafePointerFromUInt(offset));
}

uint64_t GLESv2Decoder::s_glFenceSyncAEMU(void* self, GLenum condition, GLbitfield flags) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    return (uint64_t)(uintptr_t)ctx->glFenceSync(condition, flags);
}

GLenum GLESv2Decoder::s_glClientWaitSyncAEMU(void* self, uint64_t wait_on, GLbitfield flags, GLuint64 timeout) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    return ctx->glClientWaitSync((GLsync)(uintptr_t)wait_on, flags, timeout);
}

void GLESv2Decoder::s_glWaitSyncAEMU(void* self, uint64_t wait_on, GLbitfield flags, GLuint64 timeout) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glWaitSync((GLsync)(uintptr_t)wait_on, flags, timeout);
}

void GLESv2Decoder::s_glDeleteSyncAEMU(void* self, uint64_t to_delete) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glDeleteSync((GLsync)(uintptr_t)to_delete);
}

GLboolean GLESv2Decoder::s_glIsSyncAEMU(void* self, uint64_t sync) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    return ctx->glIsSync((GLsync)(uintptr_t)sync);
}

void GLESv2Decoder::s_glGetSyncivAEMU(void* self, uint64_t sync, GLenum pname, GLsizei bufSize, GLsizei *length, GLint *values) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glGetSynciv((GLsync)(uintptr_t)sync, pname, bufSize, length, values);
}

GLuint GLESv2Decoder::s_glCreateShader(void* self, GLenum shaderType) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    GLuint shader = ctx->glCreateShader(shaderType);
#if 0
    if (ctx->m_snapshot) {
        GLuint emuName = ctx->m_snapshot->createShader(shader, shaderType);
        return emuName;
    }
#endif
    return shader;
}

GLuint GLESv2Decoder::s_glCreateProgram(void* self) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    return ctx->glCreateProgram();
}

void GLESv2Decoder::s_glGenBuffers(void* self, GLsizei n, GLuint* buffers) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glGenBuffers(n, buffers);
#if 0
    if (ctx->m_snapshot) {
        ctx->m_snapshot->genBuffers(n, buffers);
    }
#endif
}

void GLESv2Decoder::s_glGenFramebuffers(void* self, GLsizei n, GLuint* framebuffers) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glGenFramebuffers(n, framebuffers);
    // TODO: Snapshot names
}

void GLESv2Decoder::s_glGenRenderbuffers(void* self, GLsizei n, GLuint* renderbuffers) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glGenRenderbuffers(n, renderbuffers);
    // TODO: Snapshot names
}

void GLESv2Decoder::s_glGenTextures(void* self, GLsizei n, GLuint* textures) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glGenTextures(n, textures);
    // TODO: Snapshot names
}

void GLESv2Decoder::s_glGenVertexArraysOES(void* self, GLsizei n, GLuint* arrays) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glGenVertexArraysOES(n, arrays);
    // TODO: Snapshot names
}

void GLESv2Decoder::s_glGenVertexArrays(void* self, GLsizei n, GLuint* arrays) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glGenVertexArrays(n, arrays);
    // TODO: Snapshot names
}

void GLESv2Decoder::s_glGenTransformFeedbacks(void* self, GLsizei n, GLuint* transformFeedbacks) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glGenTransformFeedbacks(n, transformFeedbacks);
    // TODO: Snapshot names
}

void GLESv2Decoder::s_glGenSamplers(void* self, GLsizei n, GLuint* samplers) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glGenSamplers(n, samplers);
    // TODO: Snapshot names

}

void GLESv2Decoder::s_glGenQueries(void* self, GLsizei n, GLuint* queries) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glGenQueries(n, queries);
    // TODO: Snapshot names

}

void GLESv2Decoder::s_glGenProgramPipelines(void* self, GLsizei n, GLuint* pipelines) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glGenProgramPipelines(n, pipelines);
    // TODO: Snapshot names
}

void GLESv2Decoder::s_glDeleteShader(void* self, GLuint shader) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glDeleteShader(shader);
    // TODO: Snapshot names
}

void GLESv2Decoder::s_glDeleteProgram(void* self, GLuint program) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glDeleteProgram(program);
    // TODO: Snapshot names
}

void GLESv2Decoder::s_glDeleteBuffers(void* self, GLsizei n, const GLuint *buffers) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glDeleteBuffers(n, buffers);
    // TODO: Snapshot names
}

void GLESv2Decoder::s_glDeleteFramebuffers(void* self, GLsizei n, const GLuint *framebuffers) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glDeleteFramebuffers(n, framebuffers);
    // TODO: Snapshot names
}

void GLESv2Decoder::s_glDeleteRenderbuffers(void* self, GLsizei n, const GLuint *renderbuffers) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glDeleteRenderbuffers(n, renderbuffers);
    // TODO: Snapshot names
}

void GLESv2Decoder::s_glDeleteTextures(void* self, GLsizei n, const GLuint *textures) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glDeleteTextures(n, textures);
    // TODO: Snapshot names
}


void GLESv2Decoder::s_glDeleteVertexArraysOES(void* self, GLsizei n, const GLuint *arrays) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glDeleteVertexArraysOES(n, arrays);
    // TODO: Snapshot names
}

void GLESv2Decoder::s_glDeleteVertexArrays(void* self, GLsizei n, const GLuint *arrays) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glDeleteVertexArrays(n, arrays);
    // TODO: Snapshot names
}


void GLESv2Decoder::s_glDeleteTransformFeedbacks(void* self, GLsizei n, const GLuint *transformFeedbacks) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glDeleteTransformFeedbacks(n, transformFeedbacks);
    // TODO: Snapshot names
}

void GLESv2Decoder::s_glDeleteSamplers(void* self, GLsizei n, const GLuint *samplers) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glDeleteSamplers(n, samplers);
    // TODO: Snapshot names
}

void GLESv2Decoder::s_glDeleteQueries(void* self, GLsizei n, const GLuint *queries) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glDeleteQueries(n, queries);
    // TODO: Snapshot names
}

void GLESv2Decoder::s_glDeleteProgramPipelines(void* self, GLsizei n, const GLuint *pipelines) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glDeleteProgramPipelines(n, pipelines);
    // TODO: Snapshot names
}
#if 0
#define SNAPSHOT_PROGRAM_NAME(x) \
    GLESv2Decoder *ctx = (GLESv2Decoder *)self; \
    if (ctx->m_snapshot) { x = ctx->m_snapshot->getProgramName(x); } \
    
#define SNAPSHOT_PROGRAM_NAME2(x,y) \
    GLESv2Decoder *ctx = (GLESv2Decoder *)self; \
    if (ctx->m_snapshot) { \
        x = ctx->m_snapshot->getProgramName(x); \
        y = ctx->m_snapshot->getProgramName(y); \
    } \

#define SNAPSHOT_SHADER_CALL(funcname,argtypes,args) \
void GLESv2Decoder::s_##funcname argtypes { \
    SNAPSHOT_PROGRAM_NAME(shader) \
    ctx-> funcname args ; \
} \

#define SNAPSHOT_PROGRAM_CALL(funcname,argtypes,args) \
void GLESv2Decoder::s_##funcname argtypes  { \
    SNAPSHOT_PROGRAM_NAME(program) \
    ctx-> funcname args ; \
} \

#define SNAPSHOT_PROGRAM_CALL_RET(rettype, funcname, argtypes, args) \
rettype GLESv2Decoder::s_##funcname argtypes  { \
    SNAPSHOT_PROGRAM_NAME(program) \
    return ctx-> funcname args; \
} \

#else
#define SNAPSHOT_PROGRAM_NAME(x) \
    GLESv2Decoder *ctx = (GLESv2Decoder *)self; \
    if (ctx->m_snapshot) {  } \
    
#define SNAPSHOT_PROGRAM_NAME2(x,y) \
    GLESv2Decoder *ctx = (GLESv2Decoder *)self; \
    if (ctx->m_snapshot) { \
         \
         \
    } \

#define SNAPSHOT_SHADER_CALL(funcname,argtypes,args) \
void GLESv2Decoder::s_##funcname argtypes { \
    SNAPSHOT_PROGRAM_NAME(shader) \
    ctx-> funcname args ; \
} \

#define SNAPSHOT_PROGRAM_CALL(funcname,argtypes,args) \
void GLESv2Decoder::s_##funcname argtypes  { \
    SNAPSHOT_PROGRAM_NAME(program) \
    ctx-> funcname args ; \
} \

#define SNAPSHOT_PROGRAM_CALL_RET(rettype, funcname, argtypes, args) \
rettype GLESv2Decoder::s_##funcname argtypes  { \
    SNAPSHOT_PROGRAM_NAME(program) \
    return ctx-> funcname args; \
} \

 
#endif

void GLESv2Decoder::s_glShaderString(void *self, GLuint shader, const GLchar* string, GLsizei len)
{
    SNAPSHOT_PROGRAM_NAME(shader);

    ctx->glShaderSource(shader, 1, &string, NULL);
#if 0
    if (ctx->m_snapshot) {
        ctx->m_snapshot->shaderString(shader, string);
    }
#endif
}

void GLESv2Decoder::s_glAttachShader(void* self, GLuint program, GLuint shader) {
    SNAPSHOT_PROGRAM_NAME2(program, shader)
    ctx->glAttachShader(program, shader);
}

void GLESv2Decoder::s_glDetachShader(void* self, GLuint program, GLuint shader) {
    SNAPSHOT_PROGRAM_NAME2(program, shader)
    ctx->glDetachShader(program, shader);
}

GLboolean GLESv2Decoder::s_glIsShader(void* self, GLuint shader) {
    SNAPSHOT_PROGRAM_NAME(shader);
    return ctx->glIsShader(shader);
}

GLboolean GLESv2Decoder::s_glIsProgram(void* self, GLuint program) {
    SNAPSHOT_PROGRAM_NAME(program);
    return ctx->glIsProgram(program);
}

SNAPSHOT_SHADER_CALL(glCompileShader, (void* self,  GLuint shader), (shader))
SNAPSHOT_SHADER_CALL(glGetShaderiv, (void* self,  GLuint shader, GLenum pname, GLint* params), (shader, pname, params))
SNAPSHOT_SHADER_CALL(glGetShaderInfoLog, (void* self,  GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* infolog), (shader, bufsize, length, infolog))
SNAPSHOT_SHADER_CALL(glGetShaderSource, (void* self,  GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* source), (shader, bufsize, length, source))
SNAPSHOT_PROGRAM_CALL(glLinkProgram, (void* self,  GLuint program), (program))
SNAPSHOT_PROGRAM_CALL(glUseProgram, (void* self,  GLuint program), (program))
SNAPSHOT_PROGRAM_CALL(glValidateProgram, (void* self,  GLuint program), (program))
SNAPSHOT_PROGRAM_CALL(glGetProgramiv, (void* self,  GLuint program, GLenum pname, GLint* params), (program, pname, params))
SNAPSHOT_PROGRAM_CALL(glGetProgramInfoLog, (void* self,  GLuint program, GLsizei bufsize, GLsizei* length, GLchar* infolog), (program, bufsize, length, infolog))
SNAPSHOT_PROGRAM_CALL(glBindAttribLocation, (void* self,  GLuint program, GLuint index, const GLchar* name), (program, index, name))
SNAPSHOT_PROGRAM_CALL(glGetActiveAttrib, (void* self,  GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name), (program, index, bufsize, length, size, type, name))
SNAPSHOT_PROGRAM_CALL(glGetActiveUniform, (void* self,  GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name), (program, index, bufsize, length, size, type, name))
SNAPSHOT_PROGRAM_CALL(glGetAttachedShaders, (void* self,  GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders), (program, maxcount, count, shaders))
SNAPSHOT_PROGRAM_CALL_RET(GLint, glGetAttribLocation, (void* self,  GLuint program, const GLchar* name), (program, name))
SNAPSHOT_PROGRAM_CALL(glGetUniformfv, (void* self,  GLuint program, GLint location, GLfloat* params), (program, location, params))
SNAPSHOT_PROGRAM_CALL(glGetUniformiv, (void* self,  GLuint program, GLint location, GLint* params), (program, location, params))
SNAPSHOT_PROGRAM_CALL_RET(GLint, glGetUniformLocation, (void* self,  GLuint program, const GLchar* name), (program, name))
SNAPSHOT_PROGRAM_CALL(glGetProgramBinaryOES, (void* self,  GLuint program, GLsizei bufSize, GLsizei* length, GLenum* binaryFormat, GLvoid* binary), (program, bufSize, length, binaryFormat, binary))
SNAPSHOT_PROGRAM_CALL(glProgramBinaryOES, (void* self,  GLuint program, GLenum binaryFormat, const GLvoid* binary, GLint length), (program, binaryFormat, binary, length))
SNAPSHOT_PROGRAM_CALL(glUniformBlockBinding, (void* self,  GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding), (program, uniformBlockIndex, uniformBlockBinding))
SNAPSHOT_PROGRAM_CALL_RET(GLuint, glGetUniformBlockIndex, (void* self,  GLuint program, const GLchar* uniformBlockName), (program, uniformBlockName))
SNAPSHOT_PROGRAM_CALL(glGetActiveUniformBlockiv, (void* self,  GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint* params), (program, uniformBlockIndex, pname, params))
SNAPSHOT_PROGRAM_CALL(glGetActiveUniformBlockName, (void* self,  GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei* length, GLchar* uniformBlockName), (program, uniformBlockIndex, bufSize, length, uniformBlockName))
SNAPSHOT_PROGRAM_CALL(glGetUniformuiv, (void* self,  GLuint program, GLint location, GLuint* params), (program, location, params))
SNAPSHOT_PROGRAM_CALL(glGetActiveUniformsiv, (void* self,  GLuint program, GLsizei uniformCount, const GLuint* uniformIndices, GLenum pname, GLint* params), (program, uniformCount, uniformIndices, pname, params))
SNAPSHOT_PROGRAM_CALL(glTransformFeedbackVaryings, (void* self,  GLuint program, GLsizei count, const char** varyings, GLenum bufferMode), (program, count, varyings, bufferMode))
SNAPSHOT_PROGRAM_CALL(glGetTransformFeedbackVarying, (void* self,  GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLsizei* size, GLenum* type, char* name), (program, index, bufSize, length, size, type, name))
SNAPSHOT_PROGRAM_CALL(glProgramParameteri, (void* self,  GLuint program, GLenum pname, GLint value), (program, pname, value))
SNAPSHOT_PROGRAM_CALL(glProgramBinary, (void* self,  GLuint program, GLenum binaryFormat, const void* binary, GLsizei length), (program, binaryFormat, binary, length))
SNAPSHOT_PROGRAM_CALL(glGetProgramBinary, (void* self,  GLuint program, GLsizei bufSize, GLsizei* length, GLenum* binaryFormat, void* binary), (program, bufSize, length, binaryFormat, binary))
SNAPSHOT_PROGRAM_CALL_RET(GLint, glGetFragDataLocation, (void* self,  GLuint program, const char* name), (program, name))
SNAPSHOT_PROGRAM_CALL(glUseProgramStages, (void* self,  GLuint pipeline, GLbitfield stages, GLuint program), (pipeline, stages, program))
SNAPSHOT_PROGRAM_CALL(glActiveShaderProgram, (void* self,  GLuint pipeline, GLuint program), (pipeline, program))
SNAPSHOT_PROGRAM_CALL(glProgramUniform1f, (void* self,  GLuint program, GLint location, GLfloat v0), (program, location, v0))
SNAPSHOT_PROGRAM_CALL(glProgramUniform2f, (void* self,  GLuint program, GLint location, GLfloat v0, GLfloat v1), (program, location, v0, v1))
SNAPSHOT_PROGRAM_CALL(glProgramUniform3f, (void* self,  GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2), (program, location, v0, v1, v2))
SNAPSHOT_PROGRAM_CALL(glProgramUniform4f, (void* self,  GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3), (program, location, v0, v1, v2, v3))
SNAPSHOT_PROGRAM_CALL(glProgramUniform1i, (void* self,  GLuint program, GLint location, GLint v0), (program, location, v0))
SNAPSHOT_PROGRAM_CALL(glProgramUniform2i, (void* self,  GLuint program, GLint location, GLint v0, GLint v1), (program, location, v0, v1))
SNAPSHOT_PROGRAM_CALL(glProgramUniform3i, (void* self,  GLuint program, GLint location, GLint v0, GLint v1, GLint v2), (program, location, v0, v1, v2))
SNAPSHOT_PROGRAM_CALL(glProgramUniform4i, (void* self,  GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3), (program, location, v0, v1, v2, v3))
SNAPSHOT_PROGRAM_CALL(glProgramUniform1ui, (void* self,  GLuint program, GLint location, GLuint v0), (program, location, v0))
SNAPSHOT_PROGRAM_CALL(glProgramUniform2ui, (void* self,  GLuint program, GLint location, GLint v0, GLuint v1), (program, location, v0, v1))
SNAPSHOT_PROGRAM_CALL(glProgramUniform3ui, (void* self,  GLuint program, GLint location, GLint v0, GLint v1, GLuint v2), (program, location, v0, v1, v2))
SNAPSHOT_PROGRAM_CALL(glProgramUniform4ui, (void* self,  GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLuint v3), (program, location, v0, v1, v2, v3))
SNAPSHOT_PROGRAM_CALL(glProgramUniform1fv, (void* self,  GLuint program, GLint location, GLsizei count, const GLfloat* value), (program, location, count, value))
SNAPSHOT_PROGRAM_CALL(glProgramUniform2fv, (void* self,  GLuint program, GLint location, GLsizei count, const GLfloat* value), (program, location, count, value))
SNAPSHOT_PROGRAM_CALL(glProgramUniform3fv, (void* self,  GLuint program, GLint location, GLsizei count, const GLfloat* value), (program, location, count, value))
SNAPSHOT_PROGRAM_CALL(glProgramUniform4fv, (void* self,  GLuint program, GLint location, GLsizei count, const GLfloat* value), (program, location, count, value))
SNAPSHOT_PROGRAM_CALL(glProgramUniform1iv, (void* self,  GLuint program, GLint location, GLsizei count, const GLint* value), (program, location, count, value))
SNAPSHOT_PROGRAM_CALL(glProgramUniform2iv, (void* self,  GLuint program, GLint location, GLsizei count, const GLint* value), (program, location, count, value))
SNAPSHOT_PROGRAM_CALL(glProgramUniform3iv, (void* self,  GLuint program, GLint location, GLsizei count, const GLint* value), (program, location, count, value))
SNAPSHOT_PROGRAM_CALL(glProgramUniform4iv, (void* self,  GLuint program, GLint location, GLsizei count, const GLint* value), (program, location, count, value))
SNAPSHOT_PROGRAM_CALL(glProgramUniform1uiv, (void* self,  GLuint program, GLint location, GLsizei count, const GLuint* value), (program, location, count, value))
SNAPSHOT_PROGRAM_CALL(glProgramUniform2uiv, (void* self,  GLuint program, GLint location, GLsizei count, const GLuint* value), (program, location, count, value))
SNAPSHOT_PROGRAM_CALL(glProgramUniform3uiv, (void* self,  GLuint program, GLint location, GLsizei count, const GLuint* value), (program, location, count, value))
SNAPSHOT_PROGRAM_CALL(glProgramUniform4uiv, (void* self,  GLuint program, GLint location, GLsizei count, const GLuint* value), (program, location, count, value))
SNAPSHOT_PROGRAM_CALL(glProgramUniformMatrix2fv, (void* self,  GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value), (program, location, count, transpose, value))
SNAPSHOT_PROGRAM_CALL(glProgramUniformMatrix3fv, (void* self,  GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value), (program, location, count, transpose, value))
SNAPSHOT_PROGRAM_CALL(glProgramUniformMatrix4fv, (void* self,  GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value), (program, location, count, transpose, value))
SNAPSHOT_PROGRAM_CALL(glProgramUniformMatrix2x3fv, (void* self,  GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value), (program, location, count, transpose, value))
SNAPSHOT_PROGRAM_CALL(glProgramUniformMatrix3x2fv, (void* self,  GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value), (program, location, count, transpose, value))
SNAPSHOT_PROGRAM_CALL(glProgramUniformMatrix2x4fv, (void* self,  GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value), (program, location, count, transpose, value))
SNAPSHOT_PROGRAM_CALL(glProgramUniformMatrix4x2fv, (void* self,  GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value), (program, location, count, transpose, value))
SNAPSHOT_PROGRAM_CALL(glProgramUniformMatrix3x4fv, (void* self,  GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value), (program, location, count, transpose, value))
SNAPSHOT_PROGRAM_CALL(glProgramUniformMatrix4x3fv, (void* self,  GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value), (program, location, count, transpose, value))
SNAPSHOT_PROGRAM_CALL(glGetProgramInterfaceiv, (void* self,  GLuint program, GLenum programInterface, GLenum pname, GLint* params), (program, programInterface, pname, params))
SNAPSHOT_PROGRAM_CALL(glGetProgramResourceiv, (void* self,  GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum* props, GLsizei bufSize, GLsizei* length, GLint* params), (program, programInterface, index, propCount, props, bufSize, length, params))
SNAPSHOT_PROGRAM_CALL_RET(GLuint, glGetProgramResourceIndex, (void* self, GLuint program, GLenum programInterface, const char * name), (program, programInterface, name))
SNAPSHOT_PROGRAM_CALL_RET(GLint, glGetProgramResourceLocation, (void* self, GLuint program, GLenum programInterface, const char * name), (program, programInterface, name))
SNAPSHOT_PROGRAM_CALL(glGetProgramResourceName, (void* self,  GLuint program, GLenum programInterface, GLuint index, GLsizei bufSize, GLsizei* length, char* name), (program, programInterface, index, bufSize, length, name))
