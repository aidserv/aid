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

#include "GLESv2Context.h"

#include "android/base/files/StreamSerializing.h"
#include "SamplerData.h"
#include "ShaderParser.h"
#include "ProgramData.h"

#include <string.h>

static const char kGLES20StringPart[] = "OpenGL ES 2.0";
static const char kGLES30StringPart[] = "OpenGL ES 3.0";
static const char kGLES31StringPart[] = "OpenGL ES 3.1";
static const char kGLES32StringPart[] = "OpenGL ES 3.2";

static const char* sPickVersionStringPart(int maj, int min) {
    switch (maj) {
        case 2:
            return kGLES20StringPart;
        case 3:
            switch (min) {
                case 0:
                    return kGLES30StringPart;
                case 1:
                    return kGLES31StringPart;
                case 2:
                    return kGLES32StringPart;
                default:
                    return nullptr;
            }
        default:
            return nullptr;
    }
    return nullptr;
}

void GLESv2Context::init(GlLibrary* glLib) {
    emugl::Mutex::AutoLock mutex(s_lock);
    if(!m_initialized) {
        s_glDispatch.dispatchFuncs(GLES_2_0, glLib);
        GLEScontext::init(glLib);
        addVertexArrayObject(0);
        setVertexArrayObject(0);
        setAttribute0value(0.0, 0.0, 0.0, 1.0);

        buildStrings((const char*)dispatcher().glGetString(GL_VENDOR),
                     (const char*)dispatcher().glGetString(GL_RENDERER),
                     (const char*)dispatcher().glGetString(GL_VERSION),
                     sPickVersionStringPart(m_glesMajorVersion, m_glesMinorVersion));
        if (m_glesMajorVersion > 2) {
            // OpenGL ES assumes that colors computed by / given to shaders will be converted to / from SRGB automatically
            // by the underlying implementation.
            // Desktop OpenGL makes no such assumption, and requires glEnable(GL_FRAMEBUFFER_SRGB) for the automatic conversion
            // to work.
            // This should work in most cases: just glEnable(GL_FRAMEBUFFER_SRGB) for every context.
            // But, that's not the whole story:
            // TODO: For dEQP tests standalone, we can just glEnable GL_FRAMEBUFFER_SRGB from the beginning and
            // pass all the framebuffer blit tests. However with CTS dEQP, EGL gets failures in color clear
            // and some dEQP-GLES3 framebuffer blit tests fail.
            // So we need to start out each context with GL_FRAMEBUFFER_SRGB disabled, and then enable it depending on
            // whether or not the current draw or read framebuffer has a SRGB texture color attachment.
            dispatcher().glDisable(GL_FRAMEBUFFER_SRGB);
            // Desktop OpenGL allows one to make cube maps seamless _or not_, but
            // OpenGL ES assumes seamless cubemaps are activated 100% of the time.
            // Many dEQP cube map tests fail without this enable.
            dispatcher().glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
        }

    }
    m_initialized = true;
}

void GLESv2Context::initDefaultFBO(
        GLint width, GLint height, GLint colorFormat, GLint depthstencilFormat, GLint multisamples,
        GLuint* eglSurfaceRBColorId, GLuint* eglSurfaceRBDepthId,
        GLuint readWidth, GLint readHeight, GLint readColorFormat, GLint readDepthstencilFormat, GLint readMultisamples,
        GLuint* eglReadSurfaceRBColorId, GLuint* eglReadSurfaceRBDepthId) {
    GLEScontext::initDefaultFBO(
            width, height, colorFormat, depthstencilFormat, multisamples,
            eglSurfaceRBColorId, eglSurfaceRBDepthId,
            readWidth, readHeight, readColorFormat, readDepthstencilFormat, readMultisamples,
            eglReadSurfaceRBColorId, eglReadSurfaceRBDepthId
            );
}

GLESv2Context::GLESv2Context(int maj, int min, GlobalNameSpace* globalNameSpace,
        android::base::Stream* stream, GlLibrary* glLib)
        : GLEScontext(globalNameSpace, stream, glLib) {
    if (stream) {
        assert(maj == m_glesMajorVersion);
        assert(min == m_glesMinorVersion);
        stream->read(m_attribute0value, sizeof(m_attribute0value));
        m_attribute0valueChanged = stream->getByte();
        m_att0ArrayLength = stream->getBe32();
        if (m_att0ArrayLength != 0) {
            m_att0Array.reset(new GLfloat[4 * m_att0ArrayLength]);
            stream->read(m_att0Array.get(), sizeof(GLfloat) * 4 * m_att0ArrayLength);
        }
        m_att0NeedsDisable = stream->getByte();
        m_useProgram = stream->getBe32();
        android::base::loadCollection(stream, &m_bindSampler,
                [](android::base::Stream* stream) {
                    GLuint idx = stream->getBe32();
                    GLuint val = stream->getBe32();
                    return std::make_pair(idx, val);
                });
    } else {
        m_glesMajorVersion = maj;
        m_glesMinorVersion = min;
    }
}

GLESv2Context::~GLESv2Context() {
}

void GLESv2Context::onSave(android::base::Stream* stream) const {
    GLEScontext::onSave(stream);
    stream->write(m_attribute0value, sizeof(m_attribute0value));
    stream->putByte(m_attribute0valueChanged);
    stream->putBe32(m_att0ArrayLength);
    stream->write(m_att0Array.get(), sizeof(GLfloat) * 4 * m_att0ArrayLength);
    stream->putByte(m_att0NeedsDisable);
    stream->putBe32(m_useProgram);
    android::base::saveCollection(stream, m_bindSampler,
            [](android::base::Stream* stream,
                const std::pair<const GLenum, GLuint>& item) {
                stream->putBe32(item.first);
                stream->putBe32(item.second);
            });
}

void GLESv2Context::postLoadRestoreCtx() {
    GLDispatch& dispatcher = GLEScontext::dispatcher();
    m_useProgramData = shareGroup()->getObjectDataPtr(
            NamedObjectType::SHADER_OR_PROGRAM, m_useProgram);
    const GLuint globalProgramName = shareGroup()->getGlobalName(
            NamedObjectType::SHADER_OR_PROGRAM, m_useProgram);
    dispatcher.glUseProgram(globalProgramName);

    // vertex attribute pointers
    for (const auto& vaoIte : m_vaoStateMap) {
        if (m_glesMajorVersion >= 3) {
            if (vaoIte.first != 0) {
                genVAOName(vaoIte.first, false);
            }
            dispatcher.glBindVertexArray(getVAOGlobalName(vaoIte.first));
        }
        for (const auto& glesPointerIte : *vaoIte.second.arraysMap) {
            const GLESpointer* glesPointer = glesPointerIte.second;
            // attribute 0 are bound right before draw, no need to bind it here
            if (glesPointer->getAttribType() == GLESpointer::VALUE
                    && glesPointerIte.first == 0) {
                break;
            }
            switch (glesPointer->getAttribType()) {
                case GLESpointer::BUFFER:
                    dispatcher.glBindBuffer(GL_ARRAY_BUFFER,
                            glesPointer->getBufferName());
                    dispatcher.glVertexAttribPointer(glesPointerIte.first,
                            glesPointer->getSize(),
                            glesPointer->getType(), glesPointer->isNormalize(),
                            glesPointer->getStride(),
                            (GLvoid*)(size_t)glesPointer->getBufferOffset());
                    break;
                case GLESpointer::VALUE:
                    switch (glesPointer->getValueCount()) {
                        case 1:
                            dispatcher.glVertexAttrib1fv(glesPointerIte.first,
                                    glesPointer->getValues());
                            break;
                        case 2:
                            dispatcher.glVertexAttrib2fv(glesPointerIte.first,
                                    glesPointer->getValues());
                            break;
                        case 3:
                            dispatcher.glVertexAttrib3fv(glesPointerIte.first,
                                    glesPointer->getValues());
                            break;
                        case 4:
                            dispatcher.glVertexAttrib4fv(glesPointerIte.first,
                                    glesPointer->getValues());
                            break;
                    }
                    break;
                case GLESpointer::ARRAY:
                    // client arrays are set up right before draw calls
                    // so we do nothing here
                    break;
            }
            if (glesPointer->isEnable()) {
                dispatcher.glEnableVertexAttribArray(glesPointerIte.first);
            }
        }
    }
    if (m_glesMajorVersion >= 3) {
        dispatcher.glBindVertexArray(m_currVaoState.vaoId());
        auto bindBufferRangeFunc =
                [this](GLenum target,
                    const std::vector<BufferBinding>& bufferBindings) {
                    for (unsigned int i = 0; i < bufferBindings.size(); i++) {
                        const BufferBinding& bd = bufferBindings[i];
                        GLuint globalName = this->shareGroup()->getGlobalName(
                                NamedObjectType::VERTEXBUFFER,
                                bd.buffer);
                        this->dispatcher().glBindBufferRange(target,
                                i, globalName, bd.offset, bd.size);
                    }
                };
        bindBufferRangeFunc(GL_TRANSFORM_FEEDBACK_BUFFER,
                m_indexedTransformFeedbackBuffers);
        bindBufferRangeFunc(GL_UNIFORM_BUFFER,
                m_indexedUniformBuffers);
        bindBufferRangeFunc(GL_ATOMIC_COUNTER_BUFFER,
                m_indexedAtomicCounterBuffers);
        bindBufferRangeFunc(GL_SHADER_STORAGE_BUFFER,
                m_indexedShaderStorageBuffers);
        // buffer bindings
        auto bindBuffer = [this](GLenum target, GLuint buffer) {
            this->dispatcher().glBindBuffer(target,
                    m_shareGroup->getGlobalName(NamedObjectType::VERTEXBUFFER, buffer));
        };
        bindBuffer(GL_COPY_READ_BUFFER, m_copyReadBuffer);
        bindBuffer(GL_COPY_WRITE_BUFFER, m_copyWriteBuffer);
        bindBuffer(GL_PIXEL_PACK_BUFFER, m_pixelPackBuffer);
        bindBuffer(GL_PIXEL_UNPACK_BUFFER, m_pixelUnpackBuffer);
        bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_transformFeedbackBuffer);
        bindBuffer(GL_UNIFORM_BUFFER, m_uniformBuffer);
        bindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_atomicCounterBuffer);
        bindBuffer(GL_DISPATCH_INDIRECT_BUFFER, m_dispatchIndirectBuffer);
        bindBuffer(GL_DRAW_INDIRECT_BUFFER, m_drawIndirectBuffer);
        bindBuffer(GL_SHADER_STORAGE_BUFFER, m_shaderStorageBuffer);
        for (const auto& bindSampler : m_bindSampler) {
            dispatcher.glBindSampler(bindSampler.first,
                    shareGroup()->getGlobalName(NamedObjectType::SAMPLER,
                        bindSampler.second));
        }
    }

    GLEScontext::postLoadRestoreCtx();
}

ObjectDataPtr GLESv2Context::loadObject(NamedObjectType type,
            ObjectLocalName localName, android::base::Stream* stream) const {
    switch (type) {
        case NamedObjectType::VERTEXBUFFER:
        case NamedObjectType::TEXTURE:
        case NamedObjectType::FRAMEBUFFER:
        case NamedObjectType::RENDERBUFFER:
            return GLEScontext::loadObject(type, localName, stream);
        case NamedObjectType::SAMPLER:
            return ObjectDataPtr(new SamplerData(stream));
        case NamedObjectType::SHADER_OR_PROGRAM:
            // load the first bit to see if it is a program or shader
            switch (stream->getByte()) {
                case LOAD_PROGRAM:
                    return ObjectDataPtr(new ProgramData(stream));
                case LOAD_SHADER:
                    return ObjectDataPtr(new ShaderParser(stream));
                default:
                    fprintf(stderr, "corrupted snapshot\n");
                    assert(false);
                    return nullptr;
            }
        default:
            return nullptr;
    }
}

void GLESv2Context::setAttribValue(int idx, unsigned int count,
        const GLfloat* val) {
    m_currVaoState[idx]->setValue(count, val);
}

void GLESv2Context::setAttribute0value(float x, float y, float z, float w)
{
    m_attribute0valueChanged |=
            x != m_attribute0value[0] || y != m_attribute0value[1] ||
            z != m_attribute0value[2] || w != m_attribute0value[3];
    m_attribute0value[0] = x;
    m_attribute0value[1] = y;
    m_attribute0value[2] = z;
    m_attribute0value[3] = w;
}

bool GLESv2Context::needAtt0PreDrawValidation()
{
    m_att0NeedsDisable = false;

    int enabled = 0;
    s_glDispatch.glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
    return enabled == 0;
}

void GLESv2Context::validateAtt0PreDraw(unsigned int count)
{
    if (count == 0) {
        return;
    }

    if (count > m_att0ArrayLength) {
        const unsigned newLen = std::max(count, 2 * m_att0ArrayLength);
        m_att0Array.reset(new GLfloat[4 * newLen]);
        m_att0ArrayLength = newLen;
        m_attribute0valueChanged = true;
    }
    if (m_attribute0valueChanged) {
        for(unsigned int i = 0; i<m_att0ArrayLength; i++) {
            memcpy(m_att0Array.get()+i*4, m_attribute0value,
                    sizeof(m_attribute0value));
        }
        m_attribute0valueChanged = false;
    }

    s_glDispatch.glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0,
            m_att0Array.get());
    s_glDispatch.glEnableVertexAttribArray(0);

    m_att0NeedsDisable = true;
}

void GLESv2Context::validateAtt0PostDraw(void)
{
    if (m_att0NeedsDisable) {
        s_glDispatch.glDisableVertexAttribArray(0);
        m_att0NeedsDisable = false;
    }
}

void GLESv2Context::setupArraysPointers(GLESConversionArrays& cArrs,GLint first,GLsizei count,GLenum type,const GLvoid* indices,bool direct) {
    ArraysMap::iterator it;

    //going over all clients arrays Pointers
    for ( it=m_currVaoState.begin() ; it != m_currVaoState.end(); ++it) {
        GLenum array_id = (*it).first;
        GLESpointer* p = (*it).second;
        if (!p->isEnable() || p->getAttribType() == GLESpointer::VALUE) {
            continue;
        }

        setupArr(p->getArrayData(),
                 array_id,
                 p->getType(),
                 p->getSize(),
                 p->getStride(),
                 p->getNormalized(),
                 -1,
                 p->isIntPointer());
    }
}

//setting client side arr
void GLESv2Context::setupArr(const GLvoid* arr,GLenum arrayType,GLenum dataType,GLint size,GLsizei stride,GLboolean normalized, int index, bool isInt){
    // is not really a client side arr.
    if (arr == NULL) return;

    if (isInt)
        s_glDispatch.glVertexAttribIPointer(arrayType, size, dataType, stride, arr);
    else
        s_glDispatch.glVertexAttribPointer(arrayType, size, dataType, normalized, stride, arr);
}

void GLESv2Context::setVertexAttribDivisor(GLuint bindingindex, GLuint divisor) {
    m_currVaoState.bufferBindings()[bindingindex].divisor = divisor;
}

void GLESv2Context::setVertexAttribBindingIndex(GLuint attribindex, GLuint bindingindex) {
    m_currVaoState[attribindex]->setBindingIndex(bindingindex);
}

void GLESv2Context::setVertexAttribFormat(GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint reloffset, bool isInt) {
    m_currVaoState[attribindex]->setFormat(size, type, normalized == GL_TRUE ? true : false, reloffset, isInt);
}

void GLESv2Context::setBindSampler(GLuint unit, GLuint sampler) {
    m_bindSampler[unit] = sampler;
}

bool GLESv2Context::needConvert(GLESConversionArrays& cArrs,GLint first,GLsizei count,GLenum type,const GLvoid* indices,bool direct,GLESpointer* p,GLenum array_id) {

    bool usingVBO = p->getAttribType() == GLESpointer::BUFFER;
    GLenum arrType = p->getType();

    /*
     conversion is not necessary in the following cases:
      (*) array type is not fixed
    */
    if(arrType != GL_FIXED) return false;

    if(!usingVBO) {
        if (direct) {
            convertDirect(cArrs,first,count,array_id,p);
        } else {
            convertIndirect(cArrs,count,type,indices,array_id,p);
        }
    } else {
        if (direct) {
            convertDirectVBO(cArrs,first,count,array_id,p);
        } else {
            convertIndirectVBO(cArrs,count,type,indices,array_id,p);
        }
    }
    return true;
}

void GLESv2Context::setUseProgram(GLuint program,
        const ObjectDataPtr& programData) {
    m_useProgram = program;
    assert(!programData ||
            programData->getDataType() == ObjectDataType::PROGRAM_DATA);
    m_useProgramData = programData;
}

ProgramData* GLESv2Context::getUseProgram() {
    return (ProgramData*)m_useProgramData.get();
}

void GLESv2Context::initExtensionString() {
    *s_glExtensions = "GL_OES_EGL_image GL_OES_EGL_image_external GL_OES_depth24 GL_OES_depth32 GL_OES_element_index_uint "
                      "GL_OES_texture_float GL_OES_texture_float_linear "
                      "GL_OES_compressed_paletted_texture GL_OES_compressed_ETC1_RGB8_texture GL_OES_depth_texture ";
    if (s_glSupport.GL_ARB_HALF_FLOAT_PIXEL || s_glSupport.GL_NV_HALF_FLOAT)
        *s_glExtensions+="GL_OES_texture_half_float GL_OES_texture_half_float_linear ";
    if (s_glSupport.GL_EXT_PACKED_DEPTH_STENCIL)
        *s_glExtensions+="GL_OES_packed_depth_stencil ";
    if (s_glSupport.GL_ARB_HALF_FLOAT_VERTEX)
        *s_glExtensions+="GL_OES_vertex_half_float ";
    if (s_glSupport.GL_OES_STANDARD_DERIVATIVES)
        *s_glExtensions+="GL_OES_standard_derivatives ";
    if (s_glSupport.GL_OES_TEXTURE_NPOT)
        *s_glExtensions+="GL_OES_texture_npot ";
    if (s_glSupport.GL_OES_RGB8_RGBA8)
        *s_glExtensions+="GL_OES_rgb8_rgba8 ";
    if (s_glSupport.GL_EXT_color_buffer_float)
        *s_glExtensions+="GL_EXT_color_buffer_float ";
}

int GLESv2Context::getMaxTexUnits() {
    return getCaps()->maxTexImageUnits;
}

int GLESv2Context::getMaxCombinedTexUnits() {
    return getCaps()->maxCombinedTexImageUnits;
}
