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

#ifndef GLES_V2_CONTEXT_H
#define GLES_V2_CONTEXT_H

#include <GLcommon/GLDispatch.h>
#include <GLcommon/GLEScontext.h>
#include <GLcommon/ShareGroup.h>

#include <memory>

// Extra desktop-specific OpenGL enums that we need to properly emulate OpenGL ES.
#define GL_FRAMEBUFFER_SRGB 0x8DB9
#define GL_TEXTURE_CUBE_MAP_SEAMLESS 0x884F
#define GL_DEPTH_CLAMP 0x864F

class ProgramData;

class GLESv2Context : public GLEScontext{
public:
    virtual void init(GlLibrary* glLib);
    GLESv2Context(int maj, int min, GlobalNameSpace* globalNameSpace,
            android::base::Stream* stream, GlLibrary* glLib);
    virtual ~GLESv2Context();
    void setupArraysPointers(GLESConversionArrays& fArrs,GLint first,GLsizei count,GLenum type,const GLvoid* indices,bool direct);
    void setVertexAttribDivisor(GLuint bindingindex, GLuint divisor);
    void setVertexAttribBindingIndex(GLuint attribindex, GLuint bindingindex);
    void setVertexAttribFormat(GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint reloffset, bool isInt = false);
    void setBindSampler(GLuint unit, GLuint sampler);
    int  getMaxCombinedTexUnits() override;
    int  getMaxTexUnits() override;

    void setAttribValue(int idx, unsigned int count, const GLfloat* val);
    // This whole att0 thing is about a incompatibility between GLES and OpenGL.
    // GLES allows a vertex shader attribute to be in location 0 and have a
    // current value, while OpenGL is not very clear about this, which results
    // in each implementation doing something different.
    void setAttribute0value(float x, float y, float z, float w);
    bool needAtt0PreDrawValidation();
    void validateAtt0PreDraw(unsigned int count);
    void validateAtt0PostDraw(void);
    const float* getAtt0(void) const {return m_attribute0value;}
    void setUseProgram(GLuint program, const ObjectDataPtr& programData);
    ProgramData* getUseProgram();

    virtual void onSave(android::base::Stream* stream) const override;
    virtual ObjectDataPtr loadObject(NamedObjectType type,
            ObjectLocalName localName, android::base::Stream* stream) const
            override;

    virtual void initDefaultFBO(
            GLint width, GLint height, GLint colorFormat, GLint depthstencilFormat, GLint multisamples,
            GLuint* eglSurfaceRBColorId, GLuint* eglSurfaceRBDepthId,
            GLuint readWidth, GLint readHeight, GLint readColorFormat, GLint readDepthStencilFormat, GLint readMultisamples,
            GLuint* eglReadSurfaceRBColorId, GLuint* eglReadSurfaceRBDepthId) override;
protected:
    virtual void postLoadRestoreCtx();
    bool needConvert(GLESConversionArrays& fArrs,GLint first,GLsizei count,GLenum type,const GLvoid* indices,bool direct,GLESpointer* p,GLenum array_id);
private:
    void setupArr(const GLvoid* arr,GLenum arrayType,GLenum dataType,GLint size,GLsizei stride,GLboolean normalized, int pointsIndex = -1, bool isInt = false);
    void initExtensionString();

    float m_attribute0value[4] = {};
    bool m_attribute0valueChanged = true;
    std::unique_ptr<GLfloat[]> m_att0Array = {};
    unsigned int m_att0ArrayLength = 0;
    bool m_att0NeedsDisable = false;

    GLuint m_useProgram = 0;
    ObjectDataPtr m_useProgramData = {};
    std::unordered_map<GLuint, GLuint> m_bindSampler = {};
};

#endif
