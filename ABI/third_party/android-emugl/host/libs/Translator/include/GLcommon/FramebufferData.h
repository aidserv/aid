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
#ifndef _FRAMEBUFFER_DATA_H
#define _FRAMEBUFFER_DATA_H

#include "ObjectData.h"
#include <GLES/gl.h>
#include <GLES/glext.h>
#include <GLES3/gl3.h>
#include <vector>

class RenderbufferData : public ObjectData
{
public:
    RenderbufferData() : ObjectData(RENDERBUFFER_DATA) {  };
    RenderbufferData(android::base::Stream* stream);
    virtual void onSave(android::base::Stream* stream) const override;
    virtual void restore(ObjectLocalName localName,
           getGlobalName_t getGlobalName) override;
    GLuint attachedFB = 0;
    GLenum attachedPoint = 0;
    NamedObjectPtr eglImageGlobalTexObject = 0;
    GLenum internalformat = GL_RGBA8;
    GLenum hostInternalFormat = GL_RGBA8;
    GLsizei width = 0;
    GLsizei height = 0;
};

const int MAX_ATTACH_POINTS = 19;

class FramebufferData : public ObjectData
{
public:
    explicit FramebufferData(GLuint name);
    FramebufferData(android::base::Stream* stream);
    ~FramebufferData();
    virtual void onSave(android::base::Stream* stream) const override;
    virtual void postLoad(getObjDataPtr_t getObjDataPtr) override;
    virtual void restore(ObjectLocalName localName,
           getGlobalName_t getGlobalName) override;

    void setAttachment(GLenum attachment,
                       GLenum target,
                       GLuint name,
                       ObjectDataPtr obj,
                       bool takeOwnership = false);

    GLuint getAttachment(GLenum attachment,
                         GLenum *outTarget,
                         ObjectDataPtr *outObj);

    void validate(class GLEScontext* ctx);

    void setBoundAtLeastOnce() {
        m_hasBeenBound = true;
    }

    bool hasBeenBoundAtLeastOnce() const {
        return m_hasBeenBound;
    }

    void setDrawBuffers(GLsizei n, const GLenum * bufs);
    void setReadBuffers(GLenum src);
private:
    inline int attachmentPointIndex(GLenum attachment);
    void detachObject(int idx);

private:
    GLuint m_fbName = 0;
    struct attachPoint {
        GLenum target; // OGL if owned, GLES otherwise
        GLuint name; // OGL if owned, GLES otherwise
        // objType is only used in snapshot postLoad, for retrieving obj.
        // objType's data is only valid between loading and postLoad snapshot
        NamedObjectType objType;
        ObjectDataPtr obj;
        bool owned;
    } m_attachPoints[MAX_ATTACH_POINTS+1] = {};
    bool m_dirty = false;
    bool m_hasBeenBound = false;
    bool m_hasDrawBuffers = false;
    std::vector<GLenum> m_drawBuffers = {};
    GLenum m_readBuffer = GL_COLOR_ATTACHMENT0;
};

#endif
