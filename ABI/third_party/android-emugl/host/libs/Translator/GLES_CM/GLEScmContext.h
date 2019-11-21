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
#ifndef GLES_CM_CONTEX_H
#define GLES_CM_CONTEX_H

#include <GLcommon/GLDispatch.h>
#include <GLcommon/GLESpointer.h>
#include <GLcommon/GLESbuffer.h>
#include <GLcommon/GLEScontext.h>

#include <vector>
#include <string>
#include <unordered_map>

typedef std::unordered_map<GLfloat,std::vector<int> > PointSizeIndices;

class GlLibrary;

class GLEScmContext: public GLEScontext
{
public:
    virtual void init(GlLibrary* glLib);
    GLEScmContext(int maj, int min, GlobalNameSpace* globalNameSpace,
            android::base::Stream* stream);
    void setActiveTexture(GLenum tex);
    void  setClientActiveTexture(GLenum tex);
    GLenum  getActiveTexture() { return GL_TEXTURE0 + m_activeTexture;};
    GLenum  getClientActiveTexture() { return GL_TEXTURE0 + m_clientActiveTexture;};
    void setupArraysPointers(GLESConversionArrays& fArrs,GLint first,GLsizei count,GLenum type,const GLvoid* indices,bool direct);
    void drawPointsArrs(GLESConversionArrays& arrs,GLint first,GLsizei count);
    void drawPointsElems(GLESConversionArrays& arrs,GLsizei count,GLenum type,const GLvoid* indices);
    virtual const GLESpointer* getPointer(GLenum arrType);
    int  getMaxTexUnits();

    virtual bool glGetIntegerv(GLenum pname, GLint *params);
    virtual bool glGetBooleanv(GLenum pname, GLboolean *params);
    virtual bool glGetFloatv(GLenum pname, GLfloat *params);
    virtual bool glGetFixedv(GLenum pname, GLfixed *params);

    virtual void initDefaultFBO(
            GLint width, GLint height, GLint colorFormat, GLint depthstencilFormat, GLint multisamples,
            GLuint* eglSurfaceRBColorId, GLuint* eglSurfaceRBDepthId,
            GLuint readWidth, GLint readHeight, GLint readColorFormat, GLint readDepthStencilFormat, GLint readMultisamples,
            GLuint* eglReadSurfaceRBColorId, GLuint* eglReadSurfaceRBDepthId) override;
    ~GLEScmContext();
protected:

    bool needConvert(GLESConversionArrays& fArrs,GLint first,GLsizei count,GLenum type,const GLvoid* indices,bool direct,GLESpointer* p,GLenum array_id);
private:
    void setupArrayPointerHelper(GLESConversionArrays& fArrs,GLint first,GLsizei count,GLenum type,const GLvoid* indices,bool direct,GLenum array_id,GLESpointer* p);
    void setupArr(const GLvoid* arr,GLenum arrayType,GLenum dataType,GLint size,GLsizei stride,GLboolean normalized, int pointsIndex = -1, bool isInt = false);
    void drawPoints(PointSizeIndices* points);
    void drawPointsData(GLESConversionArrays& arrs,GLint first,GLsizei count,GLenum type,const GLvoid* indices_in,bool isElemsDraw);
    void initExtensionString();

    GLESpointer*          m_texCoords = nullptr;
    int                   m_pointsIndex = -1;
    unsigned int          m_clientActiveTexture = 0;
};

#endif
