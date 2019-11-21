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

#ifndef _GL2_DECODER_H_
#define _GL2_DECODER_H_

#include "gles2_dec.h"
#include "GLDecoderContextData.h"
//#include "emugl/common/shared_library.h"
#include "emugl/common/osDynLibrary.h"

//#include "GLSnapshot.h"


typedef void (gles2_APIENTRY *glVertexAttribPointerWithDataSize_server_proc_t) (GLuint, GLint, GLenum, GLboolean, GLsizei, const GLvoid*, GLsizei);

class GLESv2Decoder : public gles2_decoder_context_t
{
public:
    typedef void *(*get_proc_func_t)(const char *name, void *userData);
    GLESv2Decoder();
    ~GLESv2Decoder();
    int initGL(get_proc_func_t getProcFunc, void *getProcFuncData);
    void setContextData(GLDecoderContextData *contextData) { m_contextData = contextData; }
    glVertexAttribPointerWithDataSize_server_proc_t glVertexAttribPointerWithDataSize;
protected:
    // GLSnapshot::GLSnapshotState *m_snapshot;
    void *m_snapshot;
private:
    GLDecoderContextData *m_contextData;
    // emugl::SharedLibrary* m_GL2library;
	osUtils::dynLibrary* m_GL2library;

    static void *s_getProc(const char *name, void *userData);
    static void gles2_APIENTRY s_glGetCompressedTextureFormats(void *self, int count, GLint *formats);
    static void gles2_APIENTRY s_glVertexAttribPointerData(void *self, GLuint indx, GLint size, GLenum type,
                                      GLboolean normalized, GLsizei stride,  void * data, GLuint datalen);
    static void gles2_APIENTRY s_glVertexAttribPointerOffset(void *self, GLuint indx, GLint size, GLenum type,
                                        GLboolean normalized, GLsizei stride,  GLuint offset);

    static void gles2_APIENTRY s_glDrawElementsOffset(void *self, GLenum mode, GLsizei count, GLenum type, GLuint offset);
    static void gles2_APIENTRY s_glDrawElementsData(void *self, GLenum mode, GLsizei count, GLenum type, void * data, GLuint datalen);
    static int  gles2_APIENTRY s_glFinishRoundTrip(void *self);

    static void gles2_APIENTRY s_glMapBufferRangeAEMU(void* self, GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access, void* mapped);
    static void gles2_APIENTRY s_glUnmapBufferAEMU(void* self, GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access, void* guest_buffer, GLboolean* out_res);
    static void gles2_APIENTRY s_glFlushMappedBufferRangeAEMU(void* self, GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access, void* guest_buffer);

    static void gles2_APIENTRY s_glCompressedTexImage2DOffsetAEMU(void* self, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, GLuint offset);
    static void gles2_APIENTRY s_glCompressedTexSubImage2DOffsetAEMU(void* self, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, GLuint offset);
    static void gles2_APIENTRY s_glTexImage2DOffsetAEMU(void* self, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, GLuint offset);
    static void gles2_APIENTRY s_glTexSubImage2DOffsetAEMU(void* self, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, GLuint offset);

    static void gles2_APIENTRY s_glVertexAttribIPointerDataAEMU(void *self, GLuint indx, GLint size, GLenum type, GLsizei stride, void * data, GLuint datalen);
    static void gles2_APIENTRY s_glVertexAttribIPointerOffsetAEMU(void *self, GLuint indx, GLint size, GLenum type, GLsizei stride,  GLuint offset);

    static void gles2_APIENTRY s_glTexImage3DOffsetAEMU(void* self, GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, GLuint offset);
    static void gles2_APIENTRY s_glTexSubImage3DOffsetAEMU(void* self, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, GLuint offset);
    static void gles2_APIENTRY s_glCompressedTexImage3DOffsetAEMU(void* self, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, GLuint offset);
    static void gles2_APIENTRY s_glCompressedTexSubImage3DOffsetAEMU(void* self, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, GLuint offset);
    static void gles2_APIENTRY s_glDrawElementsInstancedOffsetAEMU(void* self, GLenum mode, GLsizei count, GLenum type, GLuint offset, GLsizei primcount);
    static void gles2_APIENTRY s_glDrawElementsInstancedDataAEMU(void* self, GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei primcount, GLsizei datalen);

    static void gles2_APIENTRY s_glReadPixelsOffsetAEMU(void* self, GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLuint offset);

    static GLuint gles2_APIENTRY s_glCreateShaderProgramvAEMU(void* self, GLenum type, GLsizei count, const char* packedStrings, GLuint packedLen);

    static void gles2_APIENTRY s_glDrawArraysIndirectDataAEMU(void* self, GLenum mode, const void* indirect, GLuint datalen);
    static void gles2_APIENTRY s_glDrawArraysIndirectOffsetAEMU(void* self, GLenum mode, GLuint offset);

    static void gles2_APIENTRY s_glDrawElementsIndirectDataAEMU(void* self, GLenum mode, GLenum type, const void* indirect, GLuint datalen);
    static void gles2_APIENTRY s_glDrawElementsIndirectOffsetAEMU(void* self, GLenum mode, GLenum type, GLuint offset);

    static uint64_t gles2_APIENTRY s_glFenceSyncAEMU(void* self, GLenum condition, GLbitfield flags);
    static GLenum gles2_APIENTRY s_glClientWaitSyncAEMU(void* self, uint64_t wait_on, GLbitfield flags, GLuint64 timeout);
    static void gles2_APIENTRY s_glWaitSyncAEMU(void* self, uint64_t wait_on, GLbitfield flags, GLuint64 timeout);
    static void gles2_APIENTRY s_glDeleteSyncAEMU(void* self, uint64_t to_delete);
    static GLboolean gles2_APIENTRY s_glIsSyncAEMU(void* self, uint64_t sync);
    static void gles2_APIENTRY s_glGetSyncivAEMU(void* self, uint64_t sync, GLenum pname, GLsizei bufSize, GLsizei *length, GLint *values);

    //============================================================
    // Snapshot state
    //============================================================

    // All generations============================================
    
    static GLuint gles2_APIENTRY s_glCreateShader(void* self, GLenum shaderType);
    static GLuint gles2_APIENTRY s_glCreateProgram(void* self);

    static void gles2_APIENTRY s_glGenBuffers(void* self, GLsizei n, GLuint* buffers);

    static void gles2_APIENTRY s_glGenFramebuffers(void* self, GLsizei n, GLuint* framebuffers);
    static void gles2_APIENTRY s_glGenRenderbuffers(void* self, GLsizei n, GLuint* renderbuffers);
    static void gles2_APIENTRY s_glGenTextures(void* self, GLsizei n, GLuint* textures);

    static void gles2_APIENTRY s_glGenVertexArraysOES(void* self, GLsizei n, GLuint* arrays);
    static void gles2_APIENTRY s_glGenVertexArrays(void* self, GLsizei n, GLuint* arrays);

    static void gles2_APIENTRY s_glGenTransformFeedbacks(void* self, GLsizei n, GLuint* transformFeedbacks);

    static void gles2_APIENTRY s_glGenSamplers(void* self, GLsizei n, GLuint* samplers);

    static void gles2_APIENTRY s_glGenQueries(void* self, GLsizei n, GLuint* queries);

    static GLuint gles2_APIENTRY s_glCreateShaderProgramv(void* self, GLenum type, GLsizei count, const char** strings);

    static void gles2_APIENTRY s_glGenProgramPipelines(void* self, GLsizei n, GLuint* pipelines);

    // All deletes================================================
    static void gles2_APIENTRY s_glDeleteShader(void* self, GLuint shader);
    static void gles2_APIENTRY s_glDeleteProgram(void* self, GLuint program);

    static void gles2_APIENTRY s_glDeleteBuffers(void* self, GLsizei n, const GLuint *buffers);
    static void gles2_APIENTRY s_glDeleteFramebuffers(void* self, GLsizei n, const GLuint *framebuffers);
    static void gles2_APIENTRY s_glDeleteRenderbuffers(void* self, GLsizei n, const GLuint *renderbuffers);
    static void gles2_APIENTRY s_glDeleteTextures(void* self, GLsizei n, const GLuint *textures);

    static void gles2_APIENTRY s_glDeleteVertexArraysOES(void* self, GLsizei n, const GLuint *arrays);
    static void gles2_APIENTRY s_glDeleteVertexArrays(void* self, GLsizei n, const GLuint *arrays);

    static void gles2_APIENTRY s_glDeleteTransformFeedbacks(void* self, GLsizei n, const GLuint *transformfeedbacks);
    static void gles2_APIENTRY s_glDeleteSamplers(void* self, GLsizei n, const GLuint *samplers);
    static void gles2_APIENTRY s_glDeleteQueries(void* self, GLsizei n, const GLuint *queries);
    static void gles2_APIENTRY s_glDeleteProgramPipelines(void* self, GLsizei n, const GLuint *pipelines);

    // Shaders and programs=======================================
    static void gles2_APIENTRY s_glShaderString(void *self, GLuint shader, const GLchar* string, GLsizei len);
    static void gles2_APIENTRY s_glCompileShader(void* self, GLuint shader);
    static void gles2_APIENTRY s_glAttachShader(void* self, GLuint program, GLuint shader);
    static void gles2_APIENTRY s_glDetachShader(void* self, GLuint program, GLuint shader);
    static void gles2_APIENTRY s_glLinkProgram(void* self, GLuint program);
    static void gles2_APIENTRY s_glUseProgram(void* self, GLuint program);
    static void gles2_APIENTRY s_glValidateProgram(void* self, GLuint program);

    static GLboolean gles2_APIENTRY s_glIsShader(void* self, GLuint shader);
    static GLboolean gles2_APIENTRY s_glIsProgram(void* self, GLuint program);

    static void gles2_APIENTRY s_glGetShaderiv(void* self, GLuint shader, GLenum pname, GLint* params);
    static void gles2_APIENTRY s_glGetProgramiv(void* self, GLuint program, GLenum pname, GLint* params);
    static void gles2_APIENTRY s_glGetShaderInfoLog(void* self, GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* infolog);
    static void gles2_APIENTRY s_glGetProgramInfoLog(void* self, GLuint program, GLsizei bufsize, GLsizei* length, GLchar* infolog);
    static void gles2_APIENTRY s_glGetShaderSource(void* self, GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* source);

    static void gles2_APIENTRY s_glBindAttribLocation(void* self, GLuint program, GLuint index, const GLchar* name);
    static void gles2_APIENTRY s_glGetActiveAttrib(void* self, GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name);
    static void gles2_APIENTRY s_glGetActiveUniform(void* self, GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name);
    static void gles2_APIENTRY s_glGetAttachedShaders(void* self, GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders);
    static GLint gles2_APIENTRY s_glGetAttribLocation(void* self, GLuint program, const GLchar* name);

    static void gles2_APIENTRY s_glGetUniformfv(void* self, GLuint program, GLint location, GLfloat* params);
    static void gles2_APIENTRY s_glGetUniformiv(void* self, GLuint program, GLint location, GLint* params);
    static GLint gles2_APIENTRY s_glGetUniformLocation(void* self, GLuint program,  const GLchar* name);

    static void gles2_APIENTRY s_glGetProgramBinaryOES(void* self, GLuint program, GLsizei bufSize, GLsizei* length, GLenum* binaryFormat, GLvoid* binary);
    static void gles2_APIENTRY s_glProgramBinaryOES(void* self, GLuint program, GLenum binaryFormat, const GLvoid* binary, GLint length);

    static void gles2_APIENTRY s_glUniformBlockBinding(void* self, GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding);
    static GLuint gles2_APIENTRY s_glGetUniformBlockIndex(void* self, GLuint program, const GLchar* uniformBlockName);
    static void gles2_APIENTRY s_glGetUniformIndicesAEMU(void* self, GLuint program, GLsizei uniformCount, const GLchar* packedNames, GLsizei packedLen, GLuint* uniformIndices);

    static void gles2_APIENTRY s_glGetActiveUniformBlockiv(void* self, GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint *params);
    static void gles2_APIENTRY s_glGetActiveUniformBlockName(void* self, GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformBlockName);

    static void gles2_APIENTRY s_glGetUniformuiv(void* self, GLuint program, GLint location, GLuint *params);
    static void gles2_APIENTRY s_glGetActiveUniformsiv(void* self, GLuint program, GLsizei uniformCount, const GLuint *uniformIndices, GLenum pname, GLint *params);

    static void gles2_APIENTRY s_glTransformFeedbackVaryings(void* self, GLuint program, GLsizei count, const char ** varyings, GLenum bufferMode);
    static void gles2_APIENTRY s_glTransformFeedbackVaryingsAEMU(void* self, GLuint program, GLsizei count, const char* packedVaryings, GLuint packedVaryingsLen, GLenum bufferMode);
    static void gles2_APIENTRY s_glGetTransformFeedbackVarying(void* self, GLuint program, GLuint index, GLsizei bufSize, GLsizei * length, GLsizei * size, GLenum * type, char * name);

    static void gles2_APIENTRY s_glProgramParameteri(void* self, GLuint program, GLenum pname, GLint value);
    static void gles2_APIENTRY s_glProgramBinary(void* self, GLuint program, GLenum binaryFormat, const void *binary, GLsizei length);
    static void gles2_APIENTRY s_glGetProgramBinary(void* self, GLuint program, GLsizei bufSize, GLsizei *length, GLenum *binaryFormat, void *binary);

    static GLint gles2_APIENTRY s_glGetFragDataLocation(void* self, GLuint program, const char * name);

    static void gles2_APIENTRY s_glUseProgramStages(void* self, GLuint pipeline, GLbitfield stages, GLuint program);
    static void gles2_APIENTRY s_glActiveShaderProgram(void* self, GLuint pipeline, GLuint program);

    static void gles2_APIENTRY s_glProgramUniform1f(void* self, GLuint program, GLint location, GLfloat v0);
    static void gles2_APIENTRY s_glProgramUniform2f(void* self, GLuint program, GLint location, GLfloat v0, GLfloat v1);
    static void gles2_APIENTRY s_glProgramUniform3f(void* self, GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
    static void gles2_APIENTRY s_glProgramUniform4f(void* self, GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
    static void gles2_APIENTRY s_glProgramUniform1i(void* self, GLuint program, GLint location, GLint v0);
    static void gles2_APIENTRY s_glProgramUniform2i(void* self, GLuint program, GLint location, GLint v0, GLint v1);
    static void gles2_APIENTRY s_glProgramUniform3i(void* self, GLuint program, GLint location, GLint v0, GLint v1, GLint v2);
    static void gles2_APIENTRY s_glProgramUniform4i(void* self, GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
    static void gles2_APIENTRY s_glProgramUniform1ui(void* self, GLuint program, GLint location, GLuint v0);
    static void gles2_APIENTRY s_glProgramUniform2ui(void* self, GLuint program, GLint location, GLint v0, GLuint v1);
    static void gles2_APIENTRY s_glProgramUniform3ui(void* self, GLuint program, GLint location, GLint v0, GLint v1, GLuint v2);
    static void gles2_APIENTRY s_glProgramUniform4ui(void* self, GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLuint v3);
    static void gles2_APIENTRY s_glProgramUniform1fv(void* self, GLuint program, GLint location, GLsizei count, const GLfloat *value);
    static void gles2_APIENTRY s_glProgramUniform2fv(void* self, GLuint program, GLint location, GLsizei count, const GLfloat *value);
    static void gles2_APIENTRY s_glProgramUniform3fv(void* self, GLuint program, GLint location, GLsizei count, const GLfloat *value);
    static void gles2_APIENTRY s_glProgramUniform4fv(void* self, GLuint program, GLint location, GLsizei count, const GLfloat *value);
    static void gles2_APIENTRY s_glProgramUniform1iv(void* self, GLuint program, GLint location, GLsizei count, const GLint *value);
    static void gles2_APIENTRY s_glProgramUniform2iv(void* self, GLuint program, GLint location, GLsizei count, const GLint *value);
    static void gles2_APIENTRY s_glProgramUniform3iv(void* self, GLuint program, GLint location, GLsizei count, const GLint *value);
    static void gles2_APIENTRY s_glProgramUniform4iv(void* self, GLuint program, GLint location, GLsizei count, const GLint *value);
    static void gles2_APIENTRY s_glProgramUniform1uiv(void* self, GLuint program, GLint location, GLsizei count, const GLuint *value);
    static void gles2_APIENTRY s_glProgramUniform2uiv(void* self, GLuint program, GLint location, GLsizei count, const GLuint *value);
    static void gles2_APIENTRY s_glProgramUniform3uiv(void* self, GLuint program, GLint location, GLsizei count, const GLuint *value);
    static void gles2_APIENTRY s_glProgramUniform4uiv(void* self, GLuint program, GLint location, GLsizei count, const GLuint *value);
    static void gles2_APIENTRY s_glProgramUniformMatrix2fv(void* self, GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    static void gles2_APIENTRY s_glProgramUniformMatrix3fv(void* self, GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    static void gles2_APIENTRY s_glProgramUniformMatrix4fv(void* self, GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    static void gles2_APIENTRY s_glProgramUniformMatrix2x3fv(void* self, GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    static void gles2_APIENTRY s_glProgramUniformMatrix3x2fv(void* self, GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    static void gles2_APIENTRY s_glProgramUniformMatrix2x4fv(void* self, GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    static void gles2_APIENTRY s_glProgramUniformMatrix4x2fv(void* self, GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    static void gles2_APIENTRY s_glProgramUniformMatrix3x4fv(void* self, GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    static void gles2_APIENTRY s_glProgramUniformMatrix4x3fv(void* self, GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);

    static void gles2_APIENTRY s_glGetProgramInterfaceiv(void* self, GLuint program, GLenum programInterface, GLenum pname, GLint * params);
    static void gles2_APIENTRY s_glGetProgramResourceiv(void* self, GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum * props, GLsizei bufSize, GLsizei * length, GLint * params);
    static GLuint gles2_APIENTRY s_glGetProgramResourceIndex(void* self, GLuint program, GLenum programInterface, const char * name);
    static GLint gles2_APIENTRY s_glGetProgramResourceLocation(void* self, GLuint program, GLenum programInterface, const char * name);
    static void gles2_APIENTRY s_glGetProgramResourceName(void* self, GLuint program, GLenum programInterface, GLuint index, GLsizei bufSize, GLsizei * length, char * name);

};
#endif
