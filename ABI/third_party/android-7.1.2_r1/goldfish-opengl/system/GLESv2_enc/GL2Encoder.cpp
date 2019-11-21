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

#include "GL2Encoder.h"
#include <assert.h>
#include <ctype.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES2/gl2platform.h>

#include <GLES3/gl3.h>

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

static GLubyte *gVendorString= (GLubyte *) "Android";
static GLubyte *gRendererString= (GLubyte *) "Android HW-GLES 2.0";
static GLubyte *gVersionString= (GLubyte *) "OpenGL ES 2.0";
static GLubyte *gExtensionsString= (GLubyte *) "GL_OES_EGL_image_external ";

#define SET_ERROR_IF(condition,err) if((condition)) {                            \
        ALOGE("%s:%s:%d GL error 0x%x\n", __FILE__, __FUNCTION__, __LINE__, err); \
        ctx->setError(err);                                    \
        return;                                                  \
    }


#define RET_AND_SET_ERROR_IF(condition,err,ret) if((condition)) {                \
        ALOGE("%s:%s:%d GL error 0x%x\n", __FILE__, __FUNCTION__, __LINE__, err); \
        ctx->setError(err);                                    \
        return ret;                                              \
    }


GL2Encoder::GL2Encoder(IOStream *stream, ChecksumCalculator *protocol)
        : gl2_encoder_context_t(stream, protocol)
{
    m_initialized = false;
    m_state = NULL;
    m_error = GL_NO_ERROR;
    m_num_compressedTextureFormats = 0;
    m_max_cubeMapTextureSize = 0;
    m_max_renderBufferSize = 0;
    m_max_textureSize = 0;
    m_compressedTextureFormats = NULL;

    //overrides
#define OVERRIDE(name)  m_##name##_enc = this-> name ; this-> name = &s_##name

    OVERRIDE(glFlush);
    OVERRIDE(glPixelStorei);
    OVERRIDE(glGetString);
    OVERRIDE(glBindBuffer);
    OVERRIDE(glBufferData);
    OVERRIDE(glBufferSubData);
    OVERRIDE(glDeleteBuffers);
    OVERRIDE(glDrawArrays);
    OVERRIDE(glDrawElements);
    OVERRIDE(glGetIntegerv);
    OVERRIDE(glGetFloatv);
    OVERRIDE(glGetBooleanv);
    OVERRIDE(glVertexAttribPointer);
    OVERRIDE(glEnableVertexAttribArray);
    OVERRIDE(glDisableVertexAttribArray);
    OVERRIDE(glGetVertexAttribiv);
    OVERRIDE(glGetVertexAttribfv);
    OVERRIDE(glGetVertexAttribPointerv);

    this->glShaderBinary = &s_glShaderBinary;
    this->glShaderSource = &s_glShaderSource;
    this->glFinish = &s_glFinish;

    OVERRIDE(glGetError);
    OVERRIDE(glLinkProgram);
    OVERRIDE(glDeleteProgram);
    OVERRIDE(glGetUniformiv);
    OVERRIDE(glGetUniformfv);
    OVERRIDE(glCreateProgram);
    OVERRIDE(glCreateShader);
    OVERRIDE(glDeleteShader);
    OVERRIDE(glAttachShader);
    OVERRIDE(glDetachShader);
    OVERRIDE(glGetAttachedShaders);
    OVERRIDE(glGetShaderSource);
    OVERRIDE(glGetShaderInfoLog);
    OVERRIDE(glGetProgramInfoLog);

    OVERRIDE(glGetUniformLocation);
    OVERRIDE(glUseProgram);

    OVERRIDE(glUniform1f);
    OVERRIDE(glUniform1fv);
    OVERRIDE(glUniform1i);
    OVERRIDE(glUniform1iv);
    OVERRIDE(glUniform2f);
    OVERRIDE(glUniform2fv);
    OVERRIDE(glUniform2i);
    OVERRIDE(glUniform2iv);
    OVERRIDE(glUniform3f);
    OVERRIDE(glUniform3fv);
    OVERRIDE(glUniform3i);
    OVERRIDE(glUniform3iv);
    OVERRIDE(glUniform4f);
    OVERRIDE(glUniform4fv);
    OVERRIDE(glUniform4i);
    OVERRIDE(glUniform4iv);
    OVERRIDE(glUniformMatrix2fv);
    OVERRIDE(glUniformMatrix3fv);
    OVERRIDE(glUniformMatrix4fv);

    OVERRIDE(glActiveTexture);
    OVERRIDE(glBindTexture);
    OVERRIDE(glDeleteTextures);
    OVERRIDE(glGetTexParameterfv);
    OVERRIDE(glGetTexParameteriv);
    OVERRIDE(glTexParameterf);
    OVERRIDE(glTexParameterfv);
    OVERRIDE(glTexParameteri);
    OVERRIDE(glTexParameteriv);
    OVERRIDE(glTexImage2D);
    OVERRIDE(glTexSubImage2D);
    OVERRIDE(glCopyTexImage2D);

    OVERRIDE(glGenRenderbuffers);
    OVERRIDE(glDeleteRenderbuffers);
    OVERRIDE(glBindRenderbuffer);
    OVERRIDE(glRenderbufferStorage);
    OVERRIDE(glFramebufferRenderbuffer);

    OVERRIDE(glGenFramebuffers);
    OVERRIDE(glDeleteFramebuffers);
    OVERRIDE(glBindFramebuffer);
    OVERRIDE(glFramebufferTexture2D);
    OVERRIDE(glFramebufferTexture3DOES);
    OVERRIDE(glGetFramebufferAttachmentParameteriv);

    OVERRIDE(glCheckFramebufferStatus);
}

GL2Encoder::~GL2Encoder()
{
    delete m_compressedTextureFormats;
}

GLenum GL2Encoder::s_glGetError(void * self)
{
    GL2Encoder *ctx = (GL2Encoder *)self;
    GLenum err = ctx->getError();
    if(err != GL_NO_ERROR) {
        ctx->setError(GL_NO_ERROR);
        return err;
    }

    return ctx->m_glGetError_enc(self);

}

void GL2Encoder::s_glFlush(void *self)
{
    GL2Encoder *ctx = (GL2Encoder *) self;
    ctx->m_glFlush_enc(self);
    ctx->m_stream->flush();
}

const GLubyte *GL2Encoder::s_glGetString(void *self, GLenum name)
{
    (void)self;

    GLubyte *retval =  (GLubyte *) "";
    switch(name) {
    case GL_VENDOR:
        retval = gVendorString;
        break;
    case GL_RENDERER:
        retval = gRendererString;
        break;
    case GL_VERSION:
        retval = gVersionString;
        break;
    case GL_EXTENSIONS:
        retval = gExtensionsString;
        break;
    }
    return retval;
}

void GL2Encoder::s_glPixelStorei(void *self, GLenum param, GLint value)
{
    GL2Encoder *ctx = (GL2Encoder *)self;
    ctx->m_glPixelStorei_enc(ctx, param, value);
    assert(ctx->m_state != NULL);
    ctx->m_state->setPixelStore(param, value);
}


void GL2Encoder::s_glBindBuffer(void *self, GLenum target, GLuint id)
{
    GL2Encoder *ctx = (GL2Encoder *) self;
    assert(ctx->m_state != NULL);
    ctx->m_state->bindBuffer(target, id);
    // TODO set error state if needed;
    ctx->m_glBindBuffer_enc(self, target, id);
}

void GL2Encoder::s_glBufferData(void * self, GLenum target, GLsizeiptr size, const GLvoid * data, GLenum usage)
{
    GL2Encoder *ctx = (GL2Encoder *) self;
    SET_ERROR_IF(!(target == GL_ARRAY_BUFFER || target == GL_ELEMENT_ARRAY_BUFFER), GL_INVALID_ENUM);
    GLuint bufferId = ctx->m_state->getBuffer(target);
    SET_ERROR_IF(bufferId==0, GL_INVALID_OPERATION);
    SET_ERROR_IF(size<0, GL_INVALID_VALUE);

    ctx->m_shared->updateBufferData(bufferId, size, (void*)data);
    ctx->m_glBufferData_enc(self, target, size, data, usage);
}

void GL2Encoder::s_glBufferSubData(void * self, GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid * data)
{
    GL2Encoder *ctx = (GL2Encoder *) self;
    SET_ERROR_IF(!(target == GL_ARRAY_BUFFER || target == GL_ELEMENT_ARRAY_BUFFER), GL_INVALID_ENUM);
    GLuint bufferId = ctx->m_state->getBuffer(target);
    SET_ERROR_IF(bufferId==0, GL_INVALID_OPERATION);

    GLenum res = ctx->m_shared->subUpdateBufferData(bufferId, offset, size, (void*)data);
    SET_ERROR_IF(res, res);

    ctx->m_glBufferSubData_enc(self, target, offset, size, data);
}

void GL2Encoder::s_glDeleteBuffers(void * self, GLsizei n, const GLuint * buffers)
{
    GL2Encoder *ctx = (GL2Encoder *) self;
    SET_ERROR_IF(n<0, GL_INVALID_VALUE);
    for (int i=0; i<n; i++) {
        ctx->m_shared->deleteBufferData(buffers[i]);
        ctx->m_state->unBindBuffer(buffers[i]);
        ctx->m_glDeleteBuffers_enc(self,1,&buffers[i]);
    }
}

static bool isValidVertexAttribIndex(void *self, GLuint indx)
{
    GL2Encoder *ctx = (GL2Encoder *) self;
    GLint maxIndex;
    ctx->glGetIntegerv(self, GL_MAX_VERTEX_ATTRIBS, &maxIndex);
    return indx < maxIndex;
}

static bool isValidVertexAttribType(GLenum type)
{
    bool retval = false;
    switch (type) {
    case GL_BYTE:
    case GL_UNSIGNED_BYTE:
    case GL_SHORT:
    case GL_UNSIGNED_SHORT:
    case GL_FIXED:
    case GL_FLOAT:
    // The following are technically only available if certain GLES2 extensions are.
    // However, they are supported by desktop GL3, which is a reasonable requirement
    // for the desktop GL version. Therefore, consider them valid.
    case GL_INT:
    case GL_UNSIGNED_INT:
    case GL_HALF_FLOAT_OES:
        retval = true;
        break;
    }
    return retval;
}

void GL2Encoder::s_glVertexAttribPointer(void *self, GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid * ptr)
{
    GL2Encoder *ctx = (GL2Encoder *)self;
    assert(ctx->m_state != NULL);
    SET_ERROR_IF(!isValidVertexAttribIndex(self, indx), GL_INVALID_VALUE);
    SET_ERROR_IF((size < 1 || size > 4), GL_INVALID_VALUE);
    SET_ERROR_IF(!isValidVertexAttribType(type), GL_INVALID_ENUM);
    SET_ERROR_IF(stride < 0, GL_INVALID_VALUE);
    ctx->m_state->setState(indx, size, type, normalized, stride, ptr);
}

void GL2Encoder::s_glGetIntegerv(void *self, GLenum param, GLint *ptr)
{
    GL2Encoder *ctx = (GL2Encoder *) self;
    assert(ctx->m_state != NULL);
    GLClientState* state = ctx->m_state;

    switch (param) {
    case GL_NUM_SHADER_BINARY_FORMATS:
        *ptr = 0;
        break;
    case GL_SHADER_BINARY_FORMATS:
        // do nothing
        break;

    case GL_COMPRESSED_TEXTURE_FORMATS: {
        GLint *compressedTextureFormats = ctx->getCompressedTextureFormats();
        if (ctx->m_num_compressedTextureFormats > 0 &&
                compressedTextureFormats != NULL) {
            memcpy(ptr, compressedTextureFormats,
                    ctx->m_num_compressedTextureFormats * sizeof(GLint));
        }
        break;
    }

    case GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS:
    case GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS:
    case GL_MAX_TEXTURE_IMAGE_UNITS:
        ctx->m_glGetIntegerv_enc(self, param, ptr);
        *ptr = MIN(*ptr, GLClientState::MAX_TEXTURE_UNITS);
        break;

    case GL_TEXTURE_BINDING_2D:
        *ptr = state->getBoundTexture(GL_TEXTURE_2D);
        break;
    case GL_TEXTURE_BINDING_EXTERNAL_OES:
        *ptr = state->getBoundTexture(GL_TEXTURE_EXTERNAL_OES);
        break;

    case GL_MAX_VERTEX_ATTRIBS:
        if (!ctx->m_state->getClientStateParameter<GLint>(param, ptr)) {
            ctx->m_glGetIntegerv_enc(self, param, ptr);
            ctx->m_state->setMaxVertexAttribs(*ptr);
        }
        break;
    case GL_MAX_CUBE_MAP_TEXTURE_SIZE:
        if (ctx->m_max_cubeMapTextureSize != 0) {
            *ptr = ctx->m_max_cubeMapTextureSize;
        } else {
            ctx->m_glGetIntegerv_enc(self, param, ptr);
            ctx->m_max_cubeMapTextureSize = *ptr;
        }
        break;
    case GL_MAX_RENDERBUFFER_SIZE:
        if (ctx->m_max_renderBufferSize != 0) {
            *ptr = ctx->m_max_renderBufferSize;
        } else {
            ctx->m_glGetIntegerv_enc(self, param, ptr);
            ctx->m_max_renderBufferSize = *ptr;
        }
        break;
    case GL_MAX_TEXTURE_SIZE:
        if (ctx->m_max_textureSize != 0) {
            *ptr = ctx->m_max_textureSize;
        } else {
            ctx->m_glGetIntegerv_enc(self, param, ptr);
            ctx->m_max_textureSize = *ptr;
        }
        break;
    default:
        if (!ctx->m_state->getClientStateParameter<GLint>(param, ptr)) {
            ctx->m_glGetIntegerv_enc(self, param, ptr);
        }
        break;
    }
}


void GL2Encoder::s_glGetFloatv(void *self, GLenum param, GLfloat *ptr)
{
    GL2Encoder *ctx = (GL2Encoder *)self;
    assert(ctx->m_state != NULL);
    GLClientState* state = ctx->m_state;

    switch (param) {
    case GL_NUM_SHADER_BINARY_FORMATS:
        *ptr = 0;
        break;
    case GL_SHADER_BINARY_FORMATS:
        // do nothing
        break;

    case GL_COMPRESSED_TEXTURE_FORMATS: {
        GLint *compressedTextureFormats = ctx->getCompressedTextureFormats();
        if (ctx->m_num_compressedTextureFormats > 0 &&
                compressedTextureFormats != NULL) {
            for (int i = 0; i < ctx->m_num_compressedTextureFormats; i++) {
                ptr[i] = (GLfloat) compressedTextureFormats[i];
            }
        }
        break;
    }

    case GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS:
    case GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS:
    case GL_MAX_TEXTURE_IMAGE_UNITS:
        ctx->m_glGetFloatv_enc(self, param, ptr);
        *ptr = MIN(*ptr, (GLfloat)GLClientState::MAX_TEXTURE_UNITS);
        break;

    case GL_TEXTURE_BINDING_2D:
        *ptr = (GLfloat)state->getBoundTexture(GL_TEXTURE_2D);
        break;
    case GL_TEXTURE_BINDING_EXTERNAL_OES:
        *ptr = (GLfloat)state->getBoundTexture(GL_TEXTURE_EXTERNAL_OES);
        break;

    default:
        if (!ctx->m_state->getClientStateParameter<GLfloat>(param, ptr)) {
            ctx->m_glGetFloatv_enc(self, param, ptr);
        }
        break;
    }
}


void GL2Encoder::s_glGetBooleanv(void *self, GLenum param, GLboolean *ptr)
{
    GL2Encoder *ctx = (GL2Encoder *)self;
    assert(ctx->m_state != NULL);
    GLClientState* state = ctx->m_state;

    switch (param) {
    case GL_NUM_SHADER_BINARY_FORMATS:
        *ptr = GL_FALSE;
        break;
    case GL_SHADER_BINARY_FORMATS:
        // do nothing
        break;

    case GL_COMPRESSED_TEXTURE_FORMATS: {
        GLint *compressedTextureFormats = ctx->getCompressedTextureFormats();
        if (ctx->m_num_compressedTextureFormats > 0 &&
                compressedTextureFormats != NULL) {
            for (int i = 0; i < ctx->m_num_compressedTextureFormats; i++) {
                ptr[i] = compressedTextureFormats[i] != 0 ? GL_TRUE : GL_FALSE;
            }
        }
        break;
    }

    case GL_TEXTURE_BINDING_2D:
        *ptr = state->getBoundTexture(GL_TEXTURE_2D) != 0 ? GL_TRUE : GL_FALSE;
        break;
    case GL_TEXTURE_BINDING_EXTERNAL_OES:
        *ptr = state->getBoundTexture(GL_TEXTURE_EXTERNAL_OES) != 0
                ? GL_TRUE : GL_FALSE;
        break;

    default:
        if (!ctx->m_state->getClientStateParameter<GLboolean>(param, ptr)) {
            ctx->m_glGetBooleanv_enc(self, param, ptr);
        }
        *ptr = (*ptr != 0) ? GL_TRUE : GL_FALSE;
        break;
    }
}


void GL2Encoder::s_glEnableVertexAttribArray(void *self, GLuint index)
{
    GL2Encoder *ctx = (GL2Encoder *)self;
    assert(ctx->m_state);
    SET_ERROR_IF(!isValidVertexAttribIndex(self, index), GL_INVALID_VALUE);
    ctx->m_state->enable(index, 1);
}

void GL2Encoder::s_glDisableVertexAttribArray(void *self, GLuint index)
{
    GL2Encoder *ctx = (GL2Encoder *)self;
    assert(ctx->m_state);
    SET_ERROR_IF(!isValidVertexAttribIndex(self, index), GL_INVALID_VALUE);
    ctx->m_state->enable(index, 0);
}


void GL2Encoder::s_glGetVertexAttribiv(void *self, GLuint index, GLenum pname, GLint *params)
{
    GL2Encoder *ctx = (GL2Encoder *)self;
    assert(ctx->m_state);
    GLint maxIndex;
    ctx->glGetIntegerv(self, GL_MAX_VERTEX_ATTRIBS, &maxIndex);
    SET_ERROR_IF(!(index < maxIndex), GL_INVALID_VALUE);

    if (!ctx->m_state->getVertexAttribParameter<GLint>(index, pname, params)) {
        ctx->m_glGetVertexAttribiv_enc(self, index, pname, params);
    }
}

void GL2Encoder::s_glGetVertexAttribfv(void *self, GLuint index, GLenum pname, GLfloat *params)
{
    GL2Encoder *ctx = (GL2Encoder *)self;
    assert(ctx->m_state);
    GLint maxIndex;
    ctx->glGetIntegerv(self, GL_MAX_VERTEX_ATTRIBS, &maxIndex);
    SET_ERROR_IF(!(index < maxIndex), GL_INVALID_VALUE);

    if (!ctx->m_state->getVertexAttribParameter<GLfloat>(index, pname, params)) {
        ctx->m_glGetVertexAttribfv_enc(self, index, pname, params);
    }
}

void GL2Encoder::s_glGetVertexAttribPointerv(void *self, GLuint index, GLenum pname, GLvoid **pointer)
{
    GL2Encoder *ctx = (GL2Encoder *)self;
    if (ctx->m_state == NULL) return;
    GLint maxIndex;
    ctx->glGetIntegerv(self, GL_MAX_VERTEX_ATTRIBS, &maxIndex);
    SET_ERROR_IF(!(index < maxIndex), GL_INVALID_VALUE);
    SET_ERROR_IF(pname != GL_VERTEX_ATTRIB_ARRAY_POINTER, GL_INVALID_ENUM);
    (void)pname;

    const GLClientState::VertexAttribState *va_state = ctx->m_state->getState(index);
    if (va_state != NULL) {
        *pointer = va_state->data;
    }
}


void GL2Encoder::sendVertexAttributes(GLint first, GLsizei count)
{
    assert(m_state);

    GLuint lastBoundVbo = m_state->currentArrayVbo();
    for (int i = 0; i < m_state->nLocations(); i++) {
        bool enableDirty;
        const GLClientState::VertexAttribState *state = m_state->getStateAndEnableDirty(i, &enableDirty);

        if (!state) {
            continue;
        }

        if (!enableDirty && !state->enabled) {
            continue;
        }

        if (state->enabled) {
            if (lastBoundVbo != state->bufferObject) {
                this->m_glBindBuffer_enc(this, GL_ARRAY_BUFFER, state->bufferObject);
                lastBoundVbo = state->bufferObject;
            }

            unsigned int datalen = state->elementSize * count;
            int stride = state->stride == 0 ? state->elementSize : state->stride;
            int firstIndex = stride * first;

            if (state->bufferObject == 0) {
                m_glEnableVertexAttribArray_enc(this, i);
                this->glVertexAttribPointerData(this, i, state->size, state->type, state->normalized, state->stride,
                                                (unsigned char *)state->data + firstIndex, datalen);
            } else {
                const BufferData* buf = m_shared->getBufferData(state->bufferObject);
                // The following expression actually means bufLen = stride*count;
                // But the last element doesn't have to fill up the whole stride.
                // So it becomes the current form.
                unsigned int bufLen = stride * (count - 1) + state->elementSize;
                if (buf && firstIndex >= 0 && firstIndex + bufLen <= buf->m_size) {
                    m_glEnableVertexAttribArray_enc(this, i);
                    this->glVertexAttribPointerOffset(this, i, state->size, state->type, state->normalized, state->stride,
                                                  (uintptr_t) state->data + firstIndex);
                } else {
                    ALOGE("a vertex attribute index out of boundary is detected. Skipping corresponding vertex attribute.");
                    m_glDisableVertexAttribArray_enc(this, i);
                }
            }
        } else {
            this->m_glDisableVertexAttribArray_enc(this, i);
        }
    }

    if (lastBoundVbo != m_state->currentArrayVbo()) {
        this->m_glBindBuffer_enc(this, GL_ARRAY_BUFFER, m_state->currentArrayVbo());
    }
}

static bool isValidDrawMode(GLenum mode)
{
    bool retval = false;
    switch (mode) {
    case GL_POINTS:
    case GL_LINE_STRIP:
    case GL_LINE_LOOP:
    case GL_LINES:
    case GL_TRIANGLE_STRIP:
    case GL_TRIANGLE_FAN:
    case GL_TRIANGLES:
        retval = true;
    }
    return retval;
}

void GL2Encoder::s_glDrawArrays(void *self, GLenum mode, GLint first, GLsizei count)
{
    GL2Encoder *ctx = (GL2Encoder *)self;
    assert(ctx->m_state != NULL);
    SET_ERROR_IF(!isValidDrawMode(mode), GL_INVALID_ENUM);
    SET_ERROR_IF(count < 0, GL_INVALID_VALUE);

    bool has_arrays = false;
    int nLocations = ctx->m_state->nLocations();
    for (int i = 0; i < nLocations; i++) {
        const GLClientState::VertexAttribState *state = ctx->m_state->getState(i);
        if (state->enabled) {
            if (state->bufferObject || state->data)  {
                has_arrays = true;
            }
            else {
                ALOGE("glDrawArrays: a vertex attribute array is enabled with no data bound\n");
                ctx->setError(GL_INVALID_OPERATION);
                return;
            }
        }
    }

    ctx->sendVertexAttributes(first, count);
    ctx->m_glDrawArrays_enc(ctx, mode, 0, count);
    ctx->m_stream->flush();
}


void GL2Encoder::s_glDrawElements(void *self, GLenum mode, GLsizei count, GLenum type, const void *indices)
{

    GL2Encoder *ctx = (GL2Encoder *)self;
    assert(ctx->m_state != NULL);
    SET_ERROR_IF(!isValidDrawMode(mode), GL_INVALID_ENUM);
    SET_ERROR_IF(count < 0, GL_INVALID_VALUE);
    SET_ERROR_IF(!(type == GL_UNSIGNED_BYTE || type == GL_UNSIGNED_SHORT || type == GL_UNSIGNED_INT), GL_INVALID_ENUM);

    bool has_immediate_arrays = false;
    bool has_indirect_arrays = false;
    int nLocations = ctx->m_state->nLocations();
    GLintptr offset = 0;

    for (int i = 0; i < nLocations; i++) {
        const GLClientState::VertexAttribState *state = ctx->m_state->getState(i);
        if (state->enabled) {
            if (state->bufferObject != 0) {
                has_indirect_arrays = true;
            } else if (state->data) {
                has_immediate_arrays = true;
            } else {
                ALOGW("glDrawElements: a vertex attribute array is enabled with no data bound\n");
                ctx->setError(GL_INVALID_OPERATION);
                return;
            }
        }
    }

    if (!has_immediate_arrays && !has_indirect_arrays) {
        ALOGE("glDrawElements: no data bound to the command - ignoring\n");
        GLenum status = ctx->m_glCheckFramebufferStatus_enc(self, GL_FRAMEBUFFER);
        SET_ERROR_IF(status != GL_FRAMEBUFFER_COMPLETE, GL_INVALID_FRAMEBUFFER_OPERATION);
        return;
    }

    if (ctx->m_state->currentIndexVbo() != 0) {
        offset = (GLintptr)indices;
        BufferData * buf = ctx->m_shared->getBufferData(ctx->m_state->currentIndexVbo());
        indices = (void*)((GLintptr)buf->m_fixedBuffer.ptr() + offset);
    }
    int minIndex = 0, maxIndex = 0;
    switch(type) {
    case GL_BYTE:
    case GL_UNSIGNED_BYTE:
        GLUtils::minmax<unsigned char>((unsigned char *)indices, count, &minIndex, &maxIndex);
        break;
    case GL_SHORT:
    case GL_UNSIGNED_SHORT:
        GLUtils::minmax<unsigned short>((unsigned short *)indices, count, &minIndex, &maxIndex);
        break;
    case GL_INT:
    case GL_UNSIGNED_INT:
        GLUtils::minmax<unsigned int>((unsigned int *)indices, count, &minIndex, &maxIndex);
        break;
    default:
        ALOGE("unsupported index buffer type %d\n", type);
    }

    bool adjustIndices = true;
    if (ctx->m_state->currentIndexVbo() != 0) {
        if (!has_immediate_arrays) {
            ctx->sendVertexAttributes(0, maxIndex + 1);
            ctx->m_glBindBuffer_enc(self, GL_ELEMENT_ARRAY_BUFFER, ctx->m_state->currentIndexVbo());
            ctx->glDrawElementsOffset(ctx, mode, count, type, offset);
            ctx->m_stream->flush();
            adjustIndices = false;
        } else {
            BufferData * buf = ctx->m_shared->getBufferData(ctx->m_state->currentIndexVbo());
            ctx->m_glBindBuffer_enc(self, GL_ELEMENT_ARRAY_BUFFER, 0);
        }
    }
    if (adjustIndices) {
        void *adjustedIndices = (void*)indices;

        if (minIndex != 0) {
            adjustedIndices =  ctx->m_fixedBuffer.alloc(glSizeof(type) * count);
            switch(type) {
            case GL_BYTE:
            case GL_UNSIGNED_BYTE:
                GLUtils::shiftIndices<unsigned char>((unsigned char *)indices,
                                                 (unsigned char *)adjustedIndices,
                                                 count, -minIndex);
                break;
            case GL_SHORT:
            case GL_UNSIGNED_SHORT:
                GLUtils::shiftIndices<unsigned short>((unsigned short *)indices,
                                                  (unsigned short *)adjustedIndices,
                                                  count, -minIndex);
                break;
            case GL_INT:
            case GL_UNSIGNED_INT:
                GLUtils::shiftIndices<unsigned int>((unsigned int *)indices,
                                                 (unsigned int *)adjustedIndices,
                                                 count, -minIndex);
                break;
            default:
                ALOGE("unsupported index buffer type %d\n", type);
            }
        }
        if (has_indirect_arrays || 1) {
            ctx->sendVertexAttributes(minIndex, maxIndex - minIndex + 1);
            ctx->glDrawElementsData(ctx, mode, count, type, adjustedIndices,
                                    count * glSizeof(type));
            ctx->m_stream->flush();
            // XXX - OPTIMIZATION (see the other else branch) should be implemented
            if(!has_indirect_arrays) {
                //ALOGD("unoptimized drawelements !!!\n");
            }
        } else {
            // we are all direct arrays and immidate mode index array -
            // rebuild the arrays and the index array;
            ALOGE("glDrawElements: direct index & direct buffer data - will be implemented in later versions;\n");
        }
    }
}


GLint * GL2Encoder::getCompressedTextureFormats()
{
    if (m_compressedTextureFormats == NULL) {
        this->glGetIntegerv(this, GL_NUM_COMPRESSED_TEXTURE_FORMATS,
                            &m_num_compressedTextureFormats);
        if (m_num_compressedTextureFormats > 0) {
            // get number of texture formats;
            m_compressedTextureFormats = new GLint[m_num_compressedTextureFormats];
            this->glGetCompressedTextureFormats(this, m_num_compressedTextureFormats, m_compressedTextureFormats);
        }
    }
    return m_compressedTextureFormats;
}

// Replace uses of samplerExternalOES with sampler2D, recording the names of
// modified shaders in data. Also remove
//   #extension GL_OES_EGL_image_external : require
// statements.
//
// This implementation assumes the input has already been pre-processed. If not,
// a few cases will be mishandled:
//
// 1. "mySampler" will be incorrectly recorded as being a samplerExternalOES in
//    the following code:
//      #if 1
//      uniform sampler2D mySampler;
//      #else
//      uniform samplerExternalOES mySampler;
//      #endif
//
// 2. Comments that look like sampler declarations will be incorrectly modified
//    and recorded:
//      // samplerExternalOES hahaFooledYou
//
// 3. However, GLSL ES does not have a concatentation operator, so things like
//    this (valid in C) are invalid and not a problem:
//      #define SAMPLER(TYPE, NAME) uniform sampler#TYPE NAME
//      SAMPLER(ExternalOES, mySampler);
//
static bool replaceSamplerExternalWith2D(char* const str, ShaderData* const data)
{
    static const char STR_HASH_EXTENSION[] = "#extension";
    static const char STR_GL_OES_EGL_IMAGE_EXTERNAL[] = "GL_OES_EGL_image_external";
    static const char STR_SAMPLER_EXTERNAL_OES[] = "samplerExternalOES";
    static const char STR_SAMPLER2D_SPACE[]      = "sampler2D         ";

    // -- overwrite all "#extension GL_OES_EGL_image_external : xxx" statements
    char* c = str;
    while ((c = strstr(c, STR_HASH_EXTENSION))) {
        char* start = c;
        c += sizeof(STR_HASH_EXTENSION)-1;
        while (isspace(*c) && *c != '\0') {
            c++;
        }
        if (strncmp(c, STR_GL_OES_EGL_IMAGE_EXTERNAL,
                sizeof(STR_GL_OES_EGL_IMAGE_EXTERNAL)-1) == 0)
        {
            // #extension statements are terminated by end of line
            c = start;
            while (*c != '\0' && *c != '\r' && *c != '\n') {
                *c++ = ' ';
            }
        }
    }

    // -- replace "samplerExternalOES" with "sampler2D" and record name
    c = str;
    while ((c = strstr(c, STR_SAMPLER_EXTERNAL_OES))) {
        // Make sure "samplerExternalOES" isn't a substring of a larger token
        if (c == str || !isspace(*(c-1))) {
            c++;
            continue;
        }
        char* sampler_start = c;
        c += sizeof(STR_SAMPLER_EXTERNAL_OES)-1;
        if (!isspace(*c) && *c != '\0') {
            continue;
        }

        // capture sampler name
        while (isspace(*c) && *c != '\0') {
            c++;
        }
        if (!isalpha(*c) && *c != '_') {
            // not an identifier
            return false;
        }
        char* name_start = c;
        do {
            c++;
        } while (isalnum(*c) || *c == '_');
        data->samplerExternalNames.push_back(
                android::String8(name_start, c - name_start));

        // memcpy instead of strcpy since we don't want the NUL terminator
        memcpy(sampler_start, STR_SAMPLER2D_SPACE, sizeof(STR_SAMPLER2D_SPACE)-1);
    }

    return true;
}

void GL2Encoder::s_glShaderBinary(void *self, GLsizei n, const GLuint *shaders, GLenum binaryformat, const void* binary, GLsizei length)
{
    GL2Encoder* ctx = (GL2Encoder*)self;
    // Although it is not supported, need to set proper error code.
    SET_ERROR_IF(1, GL_INVALID_ENUM);
}

void GL2Encoder::s_glShaderSource(void *self, GLuint shader, GLsizei count, const GLchar * const *string, const GLint *length)
{
    GL2Encoder* ctx = (GL2Encoder*)self;
    ShaderData* shaderData = ctx->m_shared->getShaderData(shader);
    SET_ERROR_IF(!ctx->m_shared->isObject(shader), GL_INVALID_VALUE);
    SET_ERROR_IF(!shaderData, GL_INVALID_OPERATION);
    SET_ERROR_IF((count<0), GL_INVALID_VALUE);

    int len = glUtilsCalcShaderSourceLen((char**)string, (GLint*)length, count);
    char *str = new char[len + 1];
    glUtilsPackStrings(str, (char**)string, (GLint*)length, count);

    // TODO: pre-process str before calling replaceSamplerExternalWith2D().
    // Perhaps we can borrow Mesa's pre-processor?

    if (!replaceSamplerExternalWith2D(str, shaderData)) {
        delete[] str;
        ctx->setError(GL_OUT_OF_MEMORY);
        return;
    }

    ctx->glShaderString(ctx, shader, str, len + 1);
    delete[] str;
}

void GL2Encoder::s_glFinish(void *self)
{
    GL2Encoder *ctx = (GL2Encoder *)self;
    ctx->glFinishRoundTrip(self);
}

void GL2Encoder::s_glLinkProgram(void * self, GLuint program)
{
    GL2Encoder *ctx = (GL2Encoder *)self;
    ctx->m_glLinkProgram_enc(self, program);

    GLint linkStatus = 0;
    ctx->glGetProgramiv(self,program,GL_LINK_STATUS,&linkStatus);
    if (!linkStatus)
        return;

    //get number of active uniforms in the program
    GLint numUniforms=0;
    ctx->glGetProgramiv(self, program, GL_ACTIVE_UNIFORMS, &numUniforms);
    ctx->m_shared->initProgramData(program,numUniforms);

    //get the length of the longest uniform name
    GLint maxLength=0;
    ctx->glGetProgramiv(self, program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxLength);

    GLint size;
    GLenum type;
    GLchar *name = new GLchar[maxLength+1];
    GLint location;
    //for each active uniform, get its size and starting location.
    for (GLint i=0 ; i<numUniforms ; ++i)
    {
        ctx->glGetActiveUniform(self, program, i, maxLength, NULL, &size, &type, name);
        location = ctx->m_glGetUniformLocation_enc(self, program, name);
        ctx->m_shared->setProgramIndexInfo(program, i, location, size, type, name);
    }
    ctx->m_shared->setupLocationShiftWAR(program);

    delete[] name;
}

void GL2Encoder::s_glDeleteProgram(void *self, GLuint program)
{
    GL2Encoder *ctx = (GL2Encoder*)self;
    ctx->m_glDeleteProgram_enc(self, program);

    ctx->m_shared->deleteProgramData(program);
}

void GL2Encoder::s_glGetUniformiv(void *self, GLuint program, GLint location, GLint* params)
{
    GL2Encoder *ctx = (GL2Encoder*)self;
    SET_ERROR_IF(!ctx->m_shared->isObject(program), GL_INVALID_VALUE);
    SET_ERROR_IF(!ctx->m_shared->isProgram(program), GL_INVALID_OPERATION);
    SET_ERROR_IF(!ctx->m_shared->isProgramInitialized(program), GL_INVALID_OPERATION);
    GLint hostLoc = ctx->m_shared->locationWARAppToHost(program, location);
    SET_ERROR_IF(ctx->m_shared->getProgramUniformType(program,hostLoc)==0, GL_INVALID_OPERATION);
    ctx->m_glGetUniformiv_enc(self, program, hostLoc, params);
}
void GL2Encoder::s_glGetUniformfv(void *self, GLuint program, GLint location, GLfloat* params)
{
    GL2Encoder *ctx = (GL2Encoder*)self;
    SET_ERROR_IF(!ctx->m_shared->isObject(program), GL_INVALID_VALUE);
    SET_ERROR_IF(!ctx->m_shared->isProgram(program), GL_INVALID_OPERATION);
    SET_ERROR_IF(!ctx->m_shared->isProgramInitialized(program), GL_INVALID_OPERATION);
    GLint hostLoc = ctx->m_shared->locationWARAppToHost(program,location);
    SET_ERROR_IF(ctx->m_shared->getProgramUniformType(program,hostLoc)==0, GL_INVALID_OPERATION);
    ctx->m_glGetUniformfv_enc(self, program, hostLoc, params);
}

GLuint GL2Encoder::s_glCreateProgram(void * self)
{
    GL2Encoder *ctx = (GL2Encoder*)self;
    GLuint program = ctx->m_glCreateProgram_enc(self);
    if (program!=0)
        ctx->m_shared->addProgramData(program);
    return program;
}

GLuint GL2Encoder::s_glCreateShader(void *self, GLenum shaderType)
{
    GL2Encoder *ctx = (GL2Encoder*)self;
    RET_AND_SET_ERROR_IF(((shaderType != GL_VERTEX_SHADER) && (shaderType != GL_FRAGMENT_SHADER)),
        GL_INVALID_ENUM, 0);
    GLuint shader = ctx->m_glCreateShader_enc(self, shaderType);
    if (shader != 0) {
        if (!ctx->m_shared->addShaderData(shader)) {
            ctx->m_glDeleteShader_enc(self, shader);
            return 0;
        }
    }
    return shader;
}

void GL2Encoder::s_glGetAttachedShaders(void *self, GLuint program, GLsizei maxCount,
        GLsizei* count, GLuint* shaders)
{
    GL2Encoder *ctx = (GL2Encoder*)self;
    SET_ERROR_IF(maxCount < 0, GL_INVALID_VALUE);
    ctx->m_glGetAttachedShaders_enc(self, program, maxCount, count, shaders);
}

void GL2Encoder::s_glGetShaderSource(void *self, GLuint shader, GLsizei bufsize,
            GLsizei* length, GLchar* source)
{
    GL2Encoder *ctx = (GL2Encoder*)self;
    SET_ERROR_IF(bufsize < 0, GL_INVALID_VALUE);
    ctx->m_glGetShaderSource_enc(self, shader, bufsize, length, source);
}

void GL2Encoder::s_glGetShaderInfoLog(void *self, GLuint shader, GLsizei bufsize,
        GLsizei* length, GLchar* infolog)
{
    GL2Encoder *ctx = (GL2Encoder*)self;
    SET_ERROR_IF(bufsize < 0, GL_INVALID_VALUE);
    ctx->m_glGetShaderInfoLog_enc(self, shader, bufsize, length, infolog);
}

void GL2Encoder::s_glGetProgramInfoLog(void *self, GLuint program, GLsizei bufsize,
        GLsizei* length, GLchar* infolog)
{
    GL2Encoder *ctx = (GL2Encoder*)self;
    SET_ERROR_IF(bufsize < 0, GL_INVALID_VALUE);
    ctx->m_glGetProgramInfoLog_enc(self, program, bufsize, length, infolog);
}

void GL2Encoder::s_glDeleteShader(void *self, GLenum shader)
{
    GL2Encoder *ctx = (GL2Encoder*)self;
    ctx->m_glDeleteShader_enc(self,shader);
    ctx->m_shared->unrefShaderData(shader);
}

void GL2Encoder::s_glAttachShader(void *self, GLuint program, GLuint shader)
{
    GL2Encoder *ctx = (GL2Encoder*)self;
    ctx->m_glAttachShader_enc(self, program, shader);
    ctx->m_shared->attachShader(program, shader);
}

void GL2Encoder::s_glDetachShader(void *self, GLuint program, GLuint shader)
{
    GL2Encoder *ctx = (GL2Encoder*)self;
    ctx->m_glDetachShader_enc(self, program, shader);
    ctx->m_shared->detachShader(program, shader);
}

int GL2Encoder::s_glGetUniformLocation(void *self, GLuint program, const GLchar *name)
{
    if (!name) return -1;

    GL2Encoder *ctx = (GL2Encoder*)self;

    // if we need the uniform location WAR
    // parse array index from the end of the name string
    int arrIndex = 0;
    bool needLocationWAR = ctx->m_shared->needUniformLocationWAR(program);
    if (needLocationWAR) {
        int namelen = strlen(name);
        if (name[namelen-1] == ']') {
            const char *brace = strrchr(name,'[');
            if (!brace || sscanf(brace+1,"%d",&arrIndex) != 1) {
                return -1;
            }

        }
    }

    int hostLoc = ctx->m_glGetUniformLocation_enc(self, program, name);
    if (hostLoc >= 0 && needLocationWAR) {
        return ctx->m_shared->locationWARHostToApp(program, hostLoc, arrIndex);
    }
    return hostLoc;
}

bool GL2Encoder::updateHostTexture2DBinding(GLenum texUnit, GLenum newTarget)
{
    if (newTarget != GL_TEXTURE_2D && newTarget != GL_TEXTURE_EXTERNAL_OES)
        return false;

    m_state->setActiveTextureUnit(texUnit);

    GLenum oldTarget = m_state->getPriorityEnabledTarget(GL_TEXTURE_2D);
    if (newTarget != oldTarget) {
        if (newTarget == GL_TEXTURE_EXTERNAL_OES) {
            m_state->disableTextureTarget(GL_TEXTURE_2D);
            m_state->enableTextureTarget(GL_TEXTURE_EXTERNAL_OES);
        } else {
            m_state->disableTextureTarget(GL_TEXTURE_EXTERNAL_OES);
            m_state->enableTextureTarget(GL_TEXTURE_2D);
        }
        m_glActiveTexture_enc(this, texUnit);
        m_glBindTexture_enc(this, GL_TEXTURE_2D,
                m_state->getBoundTexture(newTarget));
        return true;
    }

    return false;
}

void GL2Encoder::s_glUseProgram(void *self, GLuint program)
{
    GL2Encoder *ctx = (GL2Encoder*)self;
    GLClientState* state = ctx->m_state;
    GLSharedGroupPtr shared = ctx->m_shared;

    SET_ERROR_IF(program && !shared->isObject(program), GL_INVALID_VALUE);
    SET_ERROR_IF(program && !shared->isProgram(program), GL_INVALID_OPERATION);

    ctx->m_glUseProgram_enc(self, program);
    ctx->m_state->setCurrentProgram(program);

    GLenum origActiveTexture = state->getActiveTextureUnit();
    GLenum hostActiveTexture = origActiveTexture;
    GLint samplerIdx = -1;
    GLint samplerVal;
    GLenum samplerTarget;
    while ((samplerIdx = shared->getNextSamplerUniform(program, samplerIdx, &samplerVal, &samplerTarget)) != -1) {
        if (samplerVal < 0 || samplerVal >= GLClientState::MAX_TEXTURE_UNITS)
            continue;
        if (ctx->updateHostTexture2DBinding(GL_TEXTURE0 + samplerVal,
                samplerTarget))
        {
            hostActiveTexture = GL_TEXTURE0 + samplerVal;
        }
    }
    state->setActiveTextureUnit(origActiveTexture);
    if (hostActiveTexture != origActiveTexture) {
        ctx->m_glActiveTexture_enc(self, origActiveTexture);
    }
}

void GL2Encoder::s_glUniform1f(void *self , GLint location, GLfloat x)
{
    GL2Encoder *ctx = (GL2Encoder*)self;
    GLint hostLoc = ctx->m_shared->locationWARAppToHost(ctx->m_state->currentProgram(),location);
    ctx->m_glUniform1f_enc(self, hostLoc, x);
}

void GL2Encoder::s_glUniform1fv(void *self , GLint location, GLsizei count, const GLfloat* v)
{
    GL2Encoder *ctx = (GL2Encoder*)self;
    GLint hostLoc = ctx->m_shared->locationWARAppToHost(ctx->m_state->currentProgram(),location);
    ctx->m_glUniform1fv_enc(self, hostLoc, count, v);
}

void GL2Encoder::s_glUniform1i(void *self , GLint location, GLint x)
{
    GL2Encoder *ctx = (GL2Encoder*)self;
    GLClientState* state = ctx->m_state;
    GLSharedGroupPtr shared = ctx->m_shared;

    GLint hostLoc = ctx->m_shared->locationWARAppToHost(ctx->m_state->currentProgram(),location);
    ctx->m_glUniform1i_enc(self, hostLoc, x);

    GLenum target;
    if (shared->setSamplerUniform(state->currentProgram(), location, x, &target)) {
        GLenum origActiveTexture = state->getActiveTextureUnit();
        if (ctx->updateHostTexture2DBinding(GL_TEXTURE0 + x, target)) {
            ctx->m_glActiveTexture_enc(self, origActiveTexture);
        }
        state->setActiveTextureUnit(origActiveTexture);
    }
}

void GL2Encoder::s_glUniform1iv(void *self , GLint location, GLsizei count, const GLint* v)
{
    GL2Encoder *ctx = (GL2Encoder*)self;
    GLint hostLoc = ctx->m_shared->locationWARAppToHost(ctx->m_state->currentProgram(),location);
    ctx->m_glUniform1iv_enc(self, hostLoc, count, v);
}

void GL2Encoder::s_glUniform2f(void *self , GLint location, GLfloat x, GLfloat y)
{
    GL2Encoder *ctx = (GL2Encoder*)self;
    GLint hostLoc = ctx->m_shared->locationWARAppToHost(ctx->m_state->currentProgram(),location);
    ctx->m_glUniform2f_enc(self, hostLoc, x, y);
}

void GL2Encoder::s_glUniform2fv(void *self , GLint location, GLsizei count, const GLfloat* v)
{
    GL2Encoder *ctx = (GL2Encoder*)self;
    GLint hostLoc = ctx->m_shared->locationWARAppToHost(ctx->m_state->currentProgram(),location);
    ctx->m_glUniform2fv_enc(self, hostLoc, count, v);
}

void GL2Encoder::s_glUniform2i(void *self , GLint location, GLint x, GLint y)
{
    GL2Encoder *ctx = (GL2Encoder*)self;
    GLint hostLoc = ctx->m_shared->locationWARAppToHost(ctx->m_state->currentProgram(),location);
    ctx->m_glUniform2i_enc(self, hostLoc, x, y);
}

void GL2Encoder::s_glUniform2iv(void *self , GLint location, GLsizei count, const GLint* v)
{
    GL2Encoder *ctx = (GL2Encoder*)self;
    GLint hostLoc = ctx->m_shared->locationWARAppToHost(ctx->m_state->currentProgram(),location);
    ctx->m_glUniform2iv_enc(self, hostLoc, count, v);
}

void GL2Encoder::s_glUniform3f(void *self , GLint location, GLfloat x, GLfloat y, GLfloat z)
{
    GL2Encoder *ctx = (GL2Encoder*)self;
    GLint hostLoc = ctx->m_shared->locationWARAppToHost(ctx->m_state->currentProgram(),location);
    ctx->m_glUniform3f_enc(self, hostLoc, x, y, z);
}

void GL2Encoder::s_glUniform3fv(void *self , GLint location, GLsizei count, const GLfloat* v)
{
    GL2Encoder *ctx = (GL2Encoder*)self;
    GLint hostLoc = ctx->m_shared->locationWARAppToHost(ctx->m_state->currentProgram(),location);
    ctx->m_glUniform3fv_enc(self, hostLoc, count, v);
}

void GL2Encoder::s_glUniform3i(void *self , GLint location, GLint x, GLint y, GLint z)
{
    GL2Encoder *ctx = (GL2Encoder*)self;
    GLint hostLoc = ctx->m_shared->locationWARAppToHost(ctx->m_state->currentProgram(),location);
    ctx->m_glUniform3i_enc(self, hostLoc, x, y, z);
}

void GL2Encoder::s_glUniform3iv(void *self , GLint location, GLsizei count, const GLint* v)
{
    GL2Encoder *ctx = (GL2Encoder*)self;
    GLint hostLoc = ctx->m_shared->locationWARAppToHost(ctx->m_state->currentProgram(),location);
    ctx->m_glUniform3iv_enc(self, hostLoc, count, v);
}

void GL2Encoder::s_glUniform4f(void *self , GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    GL2Encoder *ctx = (GL2Encoder*)self;
    GLint hostLoc = ctx->m_shared->locationWARAppToHost(ctx->m_state->currentProgram(),location);
    ctx->m_glUniform4f_enc(self, hostLoc, x, y, z, w);
}

void GL2Encoder::s_glUniform4fv(void *self , GLint location, GLsizei count, const GLfloat* v)
{
    GL2Encoder *ctx = (GL2Encoder*)self;
    GLint hostLoc = ctx->m_shared->locationWARAppToHost(ctx->m_state->currentProgram(),location);
    ctx->m_glUniform4fv_enc(self, hostLoc, count, v);
}

void GL2Encoder::s_glUniform4i(void *self , GLint location, GLint x, GLint y, GLint z, GLint w)
{
    GL2Encoder *ctx = (GL2Encoder*)self;
    GLint hostLoc = ctx->m_shared->locationWARAppToHost(ctx->m_state->currentProgram(),location);
    ctx->m_glUniform4i_enc(self, hostLoc, x, y, z, w);
}

void GL2Encoder::s_glUniform4iv(void *self , GLint location, GLsizei count, const GLint* v)
{
    GL2Encoder *ctx = (GL2Encoder*)self;
    GLint hostLoc = ctx->m_shared->locationWARAppToHost(ctx->m_state->currentProgram(),location);
    ctx->m_glUniform4iv_enc(self, hostLoc, count, v);
}

void GL2Encoder::s_glUniformMatrix2fv(void *self , GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
    GL2Encoder *ctx = (GL2Encoder*)self;
    GLint hostLoc = ctx->m_shared->locationWARAppToHost(ctx->m_state->currentProgram(),location);
    ctx->m_glUniformMatrix2fv_enc(self, hostLoc, count, transpose, value);
}

void GL2Encoder::s_glUniformMatrix3fv(void *self , GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
    GL2Encoder *ctx = (GL2Encoder*)self;
    GLint hostLoc = ctx->m_shared->locationWARAppToHost(ctx->m_state->currentProgram(),location);
    ctx->m_glUniformMatrix3fv_enc(self, hostLoc, count, transpose, value);
}

void GL2Encoder::s_glUniformMatrix4fv(void *self , GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
    GL2Encoder *ctx = (GL2Encoder*)self;
    GLint hostLoc = ctx->m_shared->locationWARAppToHost(ctx->m_state->currentProgram(),location);
    ctx->m_glUniformMatrix4fv_enc(self, hostLoc, count, transpose, value);
}

void GL2Encoder::s_glActiveTexture(void* self, GLenum texture)
{
    GL2Encoder* ctx = (GL2Encoder*)self;
    GLClientState* state = ctx->m_state;
    GLenum err;

    SET_ERROR_IF((err = state->setActiveTextureUnit(texture)) != GL_NO_ERROR, err);

    ctx->m_glActiveTexture_enc(ctx, texture);
}

void GL2Encoder::s_glBindTexture(void* self, GLenum target, GLuint texture)
{
    GL2Encoder* ctx = (GL2Encoder*)self;
    GLClientState* state = ctx->m_state;
    GLenum err;
    GLboolean firstUse;

    SET_ERROR_IF((err = state->bindTexture(target, texture, &firstUse)) != GL_NO_ERROR, err);

    if (target != GL_TEXTURE_2D && target != GL_TEXTURE_EXTERNAL_OES) {
        ctx->m_glBindTexture_enc(ctx, target, texture);
        return;
    }

    GLenum priorityTarget = state->getPriorityEnabledTarget(GL_TEXTURE_2D);

    if (target == GL_TEXTURE_EXTERNAL_OES && firstUse) {
        ctx->m_glBindTexture_enc(ctx, GL_TEXTURE_2D, texture);
        ctx->m_glTexParameteri_enc(ctx, GL_TEXTURE_2D,
                GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ctx->m_glTexParameteri_enc(ctx, GL_TEXTURE_2D,
                GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        ctx->m_glTexParameteri_enc(ctx, GL_TEXTURE_2D,
                GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        if (target != priorityTarget) {
            ctx->m_glBindTexture_enc(ctx, GL_TEXTURE_2D,
                    state->getBoundTexture(GL_TEXTURE_2D));
        }
    }

    if (target == priorityTarget) {
        ctx->m_glBindTexture_enc(ctx, GL_TEXTURE_2D, texture);
    }
}

void GL2Encoder::s_glDeleteTextures(void* self, GLsizei n, const GLuint* textures)
{
    GL2Encoder* ctx = (GL2Encoder*)self;
    GLClientState* state = ctx->m_state;

    state->deleteTextures(n, textures);
    ctx->m_glDeleteTextures_enc(ctx, n, textures);
}

void GL2Encoder::s_glGetTexParameterfv(void* self,
        GLenum target, GLenum pname, GLfloat* params)
{
    GL2Encoder* ctx = (GL2Encoder*)self;
    const GLClientState* state = ctx->m_state;

    if (target == GL_TEXTURE_2D || target == GL_TEXTURE_EXTERNAL_OES) {
        ctx->override2DTextureTarget(target);
        ctx->m_glGetTexParameterfv_enc(ctx, GL_TEXTURE_2D, pname, params);
        ctx->restore2DTextureTarget(target);
    } else {
        ctx->m_glGetTexParameterfv_enc(ctx, target, pname, params);
    }
}

void GL2Encoder::s_glGetTexParameteriv(void* self,
        GLenum target, GLenum pname, GLint* params)
{
    GL2Encoder* ctx = (GL2Encoder*)self;
    const GLClientState* state = ctx->m_state;

    switch (pname) {
    case GL_REQUIRED_TEXTURE_IMAGE_UNITS_OES:
        *params = 1;
        break;

    default:
        if (target == GL_TEXTURE_2D || target == GL_TEXTURE_EXTERNAL_OES) {
            ctx->override2DTextureTarget(target);
            ctx->m_glGetTexParameteriv_enc(ctx, GL_TEXTURE_2D, pname, params);
            ctx->restore2DTextureTarget(target);
        } else {
            ctx->m_glGetTexParameteriv_enc(ctx, target, pname, params);
        }
        break;
    }
}

static bool isValidTextureExternalParam(GLenum pname, GLenum param)
{
    switch (pname) {
    case GL_TEXTURE_MIN_FILTER:
    case GL_TEXTURE_MAG_FILTER:
        return param == GL_NEAREST || param == GL_LINEAR;

    case GL_TEXTURE_WRAP_S:
    case GL_TEXTURE_WRAP_T:
        return param == GL_CLAMP_TO_EDGE;

    default:
        return true;
    }
}

void GL2Encoder::s_glTexParameterf(void* self,
        GLenum target, GLenum pname, GLfloat param)
{
    GL2Encoder* ctx = (GL2Encoder*)self;
    const GLClientState* state = ctx->m_state;

    SET_ERROR_IF((target == GL_TEXTURE_EXTERNAL_OES &&
            !isValidTextureExternalParam(pname, (GLenum)param)),
            GL_INVALID_ENUM);

    if (target == GL_TEXTURE_2D || target == GL_TEXTURE_EXTERNAL_OES) {
        ctx->override2DTextureTarget(target);
        ctx->m_glTexParameterf_enc(ctx, GL_TEXTURE_2D, pname, param);
        ctx->restore2DTextureTarget(target);
    } else {
        ctx->m_glTexParameterf_enc(ctx, target, pname, param);
    }
}

void GL2Encoder::s_glTexParameterfv(void* self,
        GLenum target, GLenum pname, const GLfloat* params)
{
    GL2Encoder* ctx = (GL2Encoder*)self;
    const GLClientState* state = ctx->m_state;

    SET_ERROR_IF((target == GL_TEXTURE_EXTERNAL_OES &&
            !isValidTextureExternalParam(pname, (GLenum)params[0])),
            GL_INVALID_ENUM);

    if (target == GL_TEXTURE_2D || target == GL_TEXTURE_EXTERNAL_OES) {
        ctx->override2DTextureTarget(target);
        ctx->m_glTexParameterfv_enc(ctx, GL_TEXTURE_2D, pname, params);
        ctx->restore2DTextureTarget(target);
    } else {
        ctx->m_glTexParameterfv_enc(ctx, target, pname, params);
    }
}

void GL2Encoder::s_glTexParameteri(void* self,
        GLenum target, GLenum pname, GLint param)
{
    GL2Encoder* ctx = (GL2Encoder*)self;
    const GLClientState* state = ctx->m_state;

    SET_ERROR_IF((target == GL_TEXTURE_EXTERNAL_OES &&
            !isValidTextureExternalParam(pname, (GLenum)param)),
            GL_INVALID_ENUM);

    if (target == GL_TEXTURE_2D || target == GL_TEXTURE_EXTERNAL_OES) {
        ctx->override2DTextureTarget(target);
        ctx->m_glTexParameteri_enc(ctx, GL_TEXTURE_2D, pname, param);
        ctx->restore2DTextureTarget(target);
    } else {
        ctx->m_glTexParameteri_enc(ctx, target, pname, param);
    }
}

void GL2Encoder::s_glTexImage2D(void* self, GLenum target, GLint level,
        GLint internalformat, GLsizei width, GLsizei height, GLint border,
        GLenum format, GLenum type, const GLvoid* pixels)
{
    GL2Encoder* ctx = (GL2Encoder*)self;
    GLClientState* state = ctx->m_state;
    if (target == GL_TEXTURE_2D || target == GL_TEXTURE_EXTERNAL_OES) {
        ctx->override2DTextureTarget(target);
        state->setBoundTextureInternalFormat(target, internalformat);
        state->setBoundTextureFormat(target, format);
        state->setBoundTextureType(target, type);

        ctx->m_glTexImage2D_enc(ctx, target, level, internalformat, width,
                height, border, format, type, pixels);
        ctx->restore2DTextureTarget(target);
    } else {
        ctx->m_glTexImage2D_enc(ctx, target, level, internalformat, width,
                height, border, format, type, pixels);
    }
}

void GL2Encoder::s_glTexSubImage2D(void* self, GLenum target, GLint level,
        GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format,
        GLenum type, const GLvoid* pixels)
{
    GL2Encoder* ctx = (GL2Encoder*)self;

    if (target == GL_TEXTURE_2D || target == GL_TEXTURE_EXTERNAL_OES) {
        ctx->override2DTextureTarget(target);
        ctx->m_glTexSubImage2D_enc(ctx, target, level, xoffset, yoffset, width,
                height, format, type, pixels);
        ctx->restore2DTextureTarget(target);
    } else {
        ctx->m_glTexSubImage2D_enc(ctx, target, level, xoffset, yoffset, width,
                height, format, type, pixels);
    }
}

void GL2Encoder::s_glCopyTexImage2D(void* self, GLenum target, GLint level,
        GLenum internalformat, GLint x, GLint y,
        GLsizei width, GLsizei height, GLint border)
{
    GL2Encoder* ctx = (GL2Encoder*)self;
    GLClientState* state = ctx->m_state;

    // This is needed to work around underlying OpenGL drivers
    // (such as those feeding some some AMD GPUs) that expect
    // positive components of cube maps to be defined _before_
    // the negative components (otherwise a segfault occurs).
    GLenum extraTarget =
        state->copyTexImageLuminanceCubeMapAMDWorkaround
            (target, level, internalformat);

    if (extraTarget) {
        ctx->m_glCopyTexImage2D_enc(ctx, extraTarget, level, internalformat,
                                    x, y, width, height, border);
    }

    ctx->m_glCopyTexImage2D_enc(ctx, target, level, internalformat,
                                x, y, width, height, border);
}

void GL2Encoder::s_glTexParameteriv(void* self,
        GLenum target, GLenum pname, const GLint* params)
{
    GL2Encoder* ctx = (GL2Encoder*)self;
    const GLClientState* state = ctx->m_state;

    SET_ERROR_IF((target == GL_TEXTURE_EXTERNAL_OES &&
            !isValidTextureExternalParam(pname, (GLenum)params[0])),
            GL_INVALID_ENUM);

    if (target == GL_TEXTURE_2D || target == GL_TEXTURE_EXTERNAL_OES) {
        ctx->override2DTextureTarget(target);
        ctx->m_glTexParameteriv_enc(ctx, GL_TEXTURE_2D, pname, params);
        ctx->restore2DTextureTarget(target);
    } else {
        ctx->m_glTexParameteriv_enc(ctx, target, pname, params);
    }
}

bool GL2Encoder::texture2DNeedsOverride(GLenum target) const {
    return (target == GL_TEXTURE_2D || target == GL_TEXTURE_EXTERNAL_OES) &&
           target != m_state->getPriorityEnabledTarget(GL_TEXTURE_2D);
}

void GL2Encoder::override2DTextureTarget(GLenum target)
{
    if (texture2DNeedsOverride(target)) {
        m_glBindTexture_enc(this, GL_TEXTURE_2D,
                m_state->getBoundTexture(target));
    }
}

void GL2Encoder::restore2DTextureTarget(GLenum target)
{
    if (texture2DNeedsOverride(target)) {
        m_glBindTexture_enc(this, GL_TEXTURE_2D,
                m_state->getBoundTexture(
                    m_state->getPriorityEnabledTarget(GL_TEXTURE_2D)));
    }
}

void GL2Encoder::s_glGenRenderbuffers(void* self,
        GLsizei n, GLuint* renderbuffers) {
    GL2Encoder* ctx = (GL2Encoder*)self;
    GLClientState* state = ctx->m_state;

    SET_ERROR_IF(n < 0, GL_INVALID_VALUE);

    ctx->m_glGenFramebuffers_enc(self, n, renderbuffers);
    state->addRenderbuffers(n, renderbuffers);
}

void GL2Encoder::s_glDeleteRenderbuffers(void* self,
        GLsizei n, const GLuint* renderbuffers) {
    GL2Encoder* ctx = (GL2Encoder*)self;
    GLClientState* state = ctx->m_state;

    SET_ERROR_IF(n < 0, GL_INVALID_VALUE);

    ctx->m_glDeleteRenderbuffers_enc(self, n, renderbuffers);
    state->removeRenderbuffers(n, renderbuffers);
}

void GL2Encoder::s_glBindRenderbuffer(void* self,
        GLenum target, GLuint renderbuffer) {
    GL2Encoder* ctx = (GL2Encoder*)self;
    GLClientState* state = ctx->m_state;

    SET_ERROR_IF((target != GL_RENDERBUFFER),
                 GL_INVALID_ENUM);

    ctx->m_glBindRenderbuffer_enc(self, target, renderbuffer);
    state->bindRenderbuffer(target, renderbuffer);
}

void GL2Encoder::s_glRenderbufferStorage(void* self,
        GLenum target, GLenum internalformat,
        GLsizei width, GLsizei height) {
    GL2Encoder* ctx = (GL2Encoder*) self;
    GLClientState* state = ctx->m_state;

    SET_ERROR_IF(target != GL_RENDERBUFFER, GL_INVALID_ENUM);
    switch (internalformat) {
    // Funny internal formats
    // that will cause an incomplete framebuffer
    // attachment error. For dEQP,
    // we can also just abort early here in
    // RenderbufferStorage with a GL_INVALID_ENUM.
    case GL_DEPTH_COMPONENT32F:
    case GL_R8:
    case GL_R8UI:
    case GL_R8I:
    case GL_R16UI:
    case GL_R16I:
    case GL_R32UI:
    case GL_R32I:
    case GL_RG8:
    case GL_RG8UI:
    case GL_RG8I:
    case GL_RG16UI:
    case GL_RG16I:
    case GL_RG32UI:
    case GL_RG32I:
    case GL_SRGB8_ALPHA8:
    case GL_RGBA8UI:
    case GL_RGBA8I:
    case GL_RGB10_A2UI:
    case GL_RGBA16UI:
    case GL_RGBA16I:
    case GL_RGBA32I:
    case GL_RGBA32UI:
    case GL_R11F_G11F_B10F:
    case GL_R32F:
    case GL_RG32F:
    case GL_RGB32F:
    case GL_RGBA32F:
        SET_ERROR_IF(true, GL_INVALID_ENUM);
        break;
    // These 4 formats are still not OK,
    // but dEQP expects GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT or
    // GL_FRAMEBUFFER_UNSUPPORTED,
    // not a GL_INVALID_ENUM from earlier on.
    // So let's forward these to the rest of
    // FBO initialization
    case GL_R16F:
    case GL_RG16F:
    case GL_RGB16F:
    case GL_RGBA16F:
    // dEQP expects GL_FRAMEBUFFER_UNSUPPORTED or GL_FRAMEBUFFER_COMPLETE
    // for this format
    case GL_RGB10_A2:
    // These formats are OK
    case GL_DEPTH_COMPONENT16:
    case GL_DEPTH_COMPONENT24:
    case GL_DEPTH_COMPONENT32_OES:
    case GL_RGBA4:
    case GL_RGB5_A1:
    case GL_RGB565:
    case GL_RGB8_OES:
    case GL_RGBA8_OES:
    case GL_STENCIL_INDEX8:
    case GL_DEPTH32F_STENCIL8:
    case GL_DEPTH24_STENCIL8_OES:
        break;
    // Everything else: still not OK,
    // and they need the GL_INVALID_ENUM
    default:
        SET_ERROR_IF(true, GL_INVALID_ENUM);
    }

    state->setBoundRenderbufferFormat(internalformat);
    ctx->m_glRenderbufferStorage_enc(self, target, internalformat,
                                     width, height);
}

void GL2Encoder::s_glFramebufferRenderbuffer(void* self,
        GLenum target, GLenum attachment,
        GLenum renderbuffertarget, GLuint renderbuffer) {
    GL2Encoder* ctx = (GL2Encoder*)self;
    GLClientState* state = ctx->m_state;

    state->attachRbo(attachment, renderbuffer);

    ctx->m_glFramebufferRenderbuffer_enc(self, target, attachment, renderbuffertarget, renderbuffer);
}

void GL2Encoder::s_glGenFramebuffers(void* self,
        GLsizei n, GLuint* framebuffers) {
    GL2Encoder* ctx = (GL2Encoder*)self;
    GLClientState* state = ctx->m_state;

    SET_ERROR_IF(n < 0, GL_INVALID_VALUE);

    ctx->m_glGenFramebuffers_enc(self, n, framebuffers);
    state->addFramebuffers(n, framebuffers);
}

void GL2Encoder::s_glDeleteFramebuffers(void* self,
        GLsizei n, const GLuint* framebuffers) {
    GL2Encoder* ctx = (GL2Encoder*)self;
    GLClientState* state = ctx->m_state;

    SET_ERROR_IF(n < 0, GL_INVALID_VALUE);

    ctx->m_glDeleteFramebuffers_enc(self, n, framebuffers);
    state->removeFramebuffers(n, framebuffers);
}

void GL2Encoder::s_glBindFramebuffer(void* self,
        GLenum target, GLuint framebuffer) {
    GL2Encoder* ctx = (GL2Encoder*)self;
    GLClientState* state = ctx->m_state;

    SET_ERROR_IF((target != GL_FRAMEBUFFER),
                 GL_INVALID_ENUM);

    state->bindFramebuffer(target, framebuffer);

    ctx->m_glBindFramebuffer_enc(self, target, framebuffer);
}

void GL2Encoder::s_glFramebufferTexture2D(void* self,
        GLenum target, GLenum attachment,
        GLenum textarget, GLuint texture, GLint level) {
    GL2Encoder* ctx = (GL2Encoder*)self;
    GLClientState* state = ctx->m_state;

    state->attachTextureObject(attachment, texture);

    ctx->m_glFramebufferTexture2D_enc(self, target, attachment, textarget, texture, level);
}

void GL2Encoder::s_glFramebufferTexture3DOES(void* self,
        GLenum target, GLenum attachment,
        GLenum textarget, GLuint texture, GLint level, GLint zoffset) {
    GL2Encoder* ctx = (GL2Encoder*)self;
    GLClientState* state = ctx->m_state;

    state->attachTextureObject(attachment, texture);

    ctx->m_glFramebufferTexture3DOES_enc(self, target, attachment, textarget, texture, level, zoffset);
}

void GL2Encoder::s_glGetFramebufferAttachmentParameteriv(void* self,
        GLenum target, GLenum attachment, GLenum pname, GLint* params) {
    GL2Encoder* ctx = (GL2Encoder*)self;
    const GLClientState* state = ctx->m_state;

    SET_ERROR_IF(state->boundFramebuffer() == 0,
                 GL_INVALID_OPERATION);
    SET_ERROR_IF((pname != GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE) &&
                 (!state->attachmentHasObject(attachment)),
                 GL_INVALID_ENUM);

    ctx->m_glGetFramebufferAttachmentParameteriv_enc(self, target, attachment, pname, params);
}

bool GL2Encoder::isCompleteFbo(const GLClientState* state,
                               GLenum attachment) const {
    FboFormatInfo fbo_format_info;
    state->getBoundFramebufferFormat(attachment, &fbo_format_info);

    bool res;
    switch (fbo_format_info.type) {
    case FBO_ATTACHMENT_RENDERBUFFER:
        switch (fbo_format_info.rb_format) {
        case GL_R16F:
        case GL_RG16F:
        case GL_RGB16F:
        case GL_RGBA16F:
            res = false;
            break;
        case GL_STENCIL_INDEX8:
            if (attachment == GL_STENCIL_ATTACHMENT) {
                res = true;
            } else {
                res = false;
            }
            break;
        default:
            res = true;
        }
        break;
    case FBO_ATTACHMENT_TEXTURE:
        switch (fbo_format_info.tex_internalformat) {
        case GL_RED:
        case GL_RG:
        case GL_R16F:
        case GL_RG16F:
        case GL_RGBA16F:
        case GL_RGB16F:
        case GL_R11F_G11F_B10F:
            res = false;
            break;
        // No float/half-float formats allowed for RGB(A)
        case GL_RGB:
        case GL_RGBA:
            switch (fbo_format_info.tex_type) {
            case GL_FLOAT:
            case GL_HALF_FLOAT_OES:
            case GL_UNSIGNED_INT_10F_11F_11F_REV:
                res = false;
                break;
            default:
                res = true;
            }
            break;
        default:
            res = true;
        }
        break;
    case FBO_ATTACHMENT_NONE:
        res = true;
        break;
    default:
        res = true;
    }
    return res;
}

GLenum GL2Encoder::s_glCheckFramebufferStatus(void* self, GLenum target) {
    GL2Encoder* ctx = (GL2Encoder*)self;
    GLClientState* state = ctx->m_state;

    bool complete = ctx->isCompleteFbo(state, GL_COLOR_ATTACHMENT0) &&
                    ctx->isCompleteFbo(state, GL_DEPTH_ATTACHMENT) &&
                    ctx->isCompleteFbo(state, GL_STENCIL_ATTACHMENT);
    if (!complete) {
        state->setCheckFramebufferStatus(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT);
        return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
    } else {
        GLenum host_checkstatus = ctx->m_glCheckFramebufferStatus_enc(self, target);
        state->setCheckFramebufferStatus(host_checkstatus);
        return host_checkstatus;
    }
}
