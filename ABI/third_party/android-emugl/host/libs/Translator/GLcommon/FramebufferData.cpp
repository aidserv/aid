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
#include "GLcommon/FramebufferData.h"

#include "android/base/files/StreamSerializing.h"
#include "GLcommon/GLEScontext.h"

#include <GLES/gl.h>
#include <GLES/glext.h>

RenderbufferData::RenderbufferData(android::base::Stream* stream) :
    ObjectData(stream) {
    attachedFB = stream->getBe32();
    attachedPoint = stream->getBe32();
    // TODO: load eglImageGlobalTexObject
    width = stream->getBe32();
    height = stream->getBe32();
    internalformat = stream->getBe32();
    hostInternalFormat = stream->getBe32();
}

void RenderbufferData::onSave(android::base::Stream* stream) const {
    ObjectData::onSave(stream);
    stream->putBe32(attachedFB);
    stream->putBe32(attachedPoint);
    // TODO: snapshot eglImageGlobalTexObject
    if (eglImageGlobalTexObject) {
        fprintf(stderr, "RenderbufferData::onSave: warning:"
                " EglImage snapshot unimplemented. \n");
    }
    stream->putBe32(width);
    stream->putBe32(height);
    stream->putBe32(internalformat);
    stream->putBe32(hostInternalFormat);
}

void RenderbufferData::restore(ObjectLocalName localName,
           getGlobalName_t getGlobalName) {
    int globalName = getGlobalName(NamedObjectType::RENDERBUFFER,
            localName);
    GLDispatch& dispatcher = GLEScontext::dispatcher();
    dispatcher.glBindRenderbuffer(GL_RENDERBUFFER, globalName);
    dispatcher.glRenderbufferStorage(GL_RENDERBUFFER, hostInternalFormat, width,
            height);
}

static GLenum s_index2Attachment(int idx);

FramebufferData::FramebufferData(GLuint name) : ObjectData(FRAMEBUFFER_DATA)
        , m_fbName(name) {
}

FramebufferData::FramebufferData(android::base::Stream* stream) :
    ObjectData(stream) {
    m_fbName = stream->getBe32();
    int attachNum = stream->getBe32();
    (void)attachNum;
    assert(attachNum == MAX_ATTACH_POINTS);
    for (auto& attachPoint : m_attachPoints) {
        attachPoint.target = stream->getBe32();
        attachPoint.name = stream->getBe32();
        attachPoint.objType = (NamedObjectType)stream->getBe32();
        // attachPoint.obj will be set up in postLoad
        attachPoint.owned = stream->getByte();
    }
    m_dirty = stream->getByte();
    m_hasBeenBound = stream->getByte();
    m_hasDrawBuffers = stream->getByte();
    android::base::loadBuffer(stream, &m_drawBuffers);
    m_readBuffer = stream->getBe32();
}

FramebufferData::~FramebufferData() {
    for (int i=0; i<MAX_ATTACH_POINTS; i++) {
        detachObject(i);
    }
}

void FramebufferData::onSave(android::base::Stream* stream) const {
    ObjectData::onSave(stream);
    stream->putBe32(m_fbName);
    stream->putBe32(MAX_ATTACH_POINTS);
    for (auto& attachPoint : m_attachPoints) {
        stream->putBe32(attachPoint.target);
        stream->putBe32(attachPoint.name);
        // do not save attachPoint.obj
        if (attachPoint.obj) {
            stream->putBe32((uint32_t)ObjectDataType2NamedObjectType(
                    attachPoint.obj->getDataType()));
        } else {
            stream->putBe32((uint32_t)NamedObjectType::NULLTYPE);
        }
        stream->putByte(attachPoint.owned);
    }
    stream->putByte(m_dirty);
    stream->putByte(m_hasBeenBound);
    stream->putByte(m_hasDrawBuffers);
    android::base::saveBuffer(stream, m_drawBuffers);
    stream->putBe32(m_readBuffer);
}

void FramebufferData::postLoad(getObjDataPtr_t getObjDataPtr) {
    for (auto& attachPoint : m_attachPoints) {
        if (NamedObjectType::NULLTYPE != attachPoint.objType) {
            attachPoint.obj = getObjDataPtr(attachPoint.objType,
                    attachPoint.name);
            if (!attachPoint.obj) {
                fprintf(stderr, "FramebufferData::postLoad: warning: "
                        "bound render buffer restore failed.\n");
                attachPoint.obj.reset(new RenderbufferData);
            }
        } else {
            attachPoint.obj = {};
        }
    }
}

void FramebufferData::restore(ObjectLocalName localName,
           getGlobalName_t getGlobalName) {
    if (!hasBeenBoundAtLeastOnce()) return;
    int globalName = getGlobalName(NamedObjectType::FRAMEBUFFER,
            localName);
    GLDispatch& dispatcher = GLEScontext::dispatcher();
    dispatcher.glBindFramebuffer(GL_FRAMEBUFFER, globalName);
    for (int i = 0; i < MAX_ATTACH_POINTS; i++) {
        auto& attachPoint = m_attachPoints[i];
        if (!attachPoint.name) continue; // bound to nothing
        // attachPoint.owned is true only when color buffer 0 is
        // not bound. In such situation, it will generate its own object when
        // calling validate()
        if (attachPoint.owned) {
            attachPoint.name = 0;
            continue;
        }
        if (attachPoint.obj) { // binding a render buffer
            assert(attachPoint.obj->getDataType()
                    == RENDERBUFFER_DATA);
            RenderbufferData *rbData = (RenderbufferData*)attachPoint.obj.get();
            if (rbData->eglImageGlobalTexObject) {
                fprintf(stderr, "FramebufferData::restore: warning: "
                        "binding egl image unsupported\n");
            } else {
                assert(attachPoint.target == GL_RENDERBUFFER);
                dispatcher.glFramebufferRenderbufferEXT(
                        GL_FRAMEBUFFER,
                        s_index2Attachment(i),
                        attachPoint.target,
                        getGlobalName(NamedObjectType::RENDERBUFFER,
                                attachPoint.name)
                        );
            }
        } else { // binding a texture
            int texGlobalName = getGlobalName(NamedObjectType::TEXTURE,
                    attachPoint.name);
            if (!texGlobalName) {
                fprintf(stderr, "FramebufferData::restore: warning: "
                        "a texture is deleted without unbinding FBO\n");
            }
            dispatcher.glFramebufferTexture2DEXT(GL_FRAMEBUFFER,
                    s_index2Attachment(i),
                    attachPoint.target,
                    texGlobalName,
                    0);
        }
    }
    m_dirty = true;
    if (m_hasDrawBuffers) {
        dispatcher.glDrawBuffers(m_drawBuffers.size(), m_drawBuffers.data());
    }
    if (dispatcher.glReadBuffer) {
        dispatcher.glReadBuffer(m_readBuffer);
    }
}

void FramebufferData::setAttachment(GLenum attachment,
               GLenum target,
               GLuint name,
               ObjectDataPtr obj,
               bool takeOwnership) {
int idx = attachmentPointIndex(attachment);
    if (!name) {
        detachObject(idx);
        return;
    }
    if (m_attachPoints[idx].target != target ||
        m_attachPoints[idx].name != name ||
        m_attachPoints[idx].obj.get() != obj.get() ||
        m_attachPoints[idx].owned != takeOwnership) {

        detachObject(idx); 

        m_attachPoints[idx].target = target;
        m_attachPoints[idx].name = name;
        m_attachPoints[idx].obj = obj;
        m_attachPoints[idx].owned = takeOwnership;

        if (target == GL_RENDERBUFFER_OES && obj.get() != NULL) {
            RenderbufferData *rbData = (RenderbufferData *)obj.get();
            rbData->attachedFB = m_fbName;
            rbData->attachedPoint = attachment;
        }

        m_dirty = true;
    }
}

GLuint FramebufferData::getAttachment(GLenum attachment,
                 GLenum *outTarget,
                 ObjectDataPtr *outObj) {
    int idx = attachmentPointIndex(attachment);
    if (outTarget) *outTarget = m_attachPoints[idx].target;
    if (outObj) *outObj = m_attachPoints[idx].obj;
    return m_attachPoints[idx].name;
}

int FramebufferData::attachmentPointIndex(GLenum attachment)
{
    switch(attachment) {
    case GL_COLOR_ATTACHMENT0_OES:
        return 0;
    case GL_DEPTH_ATTACHMENT_OES:
        return 1;
    case GL_STENCIL_ATTACHMENT_OES:
        return 2;
    case GL_DEPTH_STENCIL_ATTACHMENT:
        return 3;
    default:
        {
            // for colorbuffer 1 ~ 15, they are continuous
            int idx = attachment - GL_COLOR_ATTACHMENT1 + 4;
            // in case for some new attachment extensions
            if (idx < 4 || idx > MAX_ATTACH_POINTS) {
                idx = MAX_ATTACH_POINTS;
            }
            return idx;
        }
    }
}

static GLenum s_index2Attachment(int idx) {
    switch (idx) {
    case 0:
        return GL_COLOR_ATTACHMENT0_OES;
    case 1:
        return GL_DEPTH_ATTACHMENT_OES;
    case 2:
        return GL_STENCIL_ATTACHMENT_OES;
    case 3:
        return GL_DEPTH_STENCIL_ATTACHMENT;
    default:
        return idx - 4 + GL_COLOR_ATTACHMENT1;
    }
}

void FramebufferData::detachObject(int idx) {
    if (m_attachPoints[idx].target == GL_RENDERBUFFER_OES && m_attachPoints[idx].obj.get() != NULL) {
        RenderbufferData *rbData = (RenderbufferData *)m_attachPoints[idx].obj.get();
        rbData->attachedFB = 0;
        rbData->attachedPoint = 0;
    }

    if(m_attachPoints[idx].owned)
    {
        switch(m_attachPoints[idx].target)
        {
        case GL_RENDERBUFFER_OES:
            GLEScontext::dispatcher().glDeleteRenderbuffersEXT(1, &(m_attachPoints[idx].name));
            break;
        case GL_TEXTURE_2D:
            GLEScontext::dispatcher().glDeleteTextures(1, &(m_attachPoints[idx].name));
            break;
        }
    }

    m_attachPoints[idx] = {};
}

void FramebufferData::validate(GLEScontext* ctx)
{
    if(!getAttachment(GL_COLOR_ATTACHMENT0_OES, NULL, NULL))
    {
        // GLES does not require the framebuffer to have a color attachment.
        // OpenGL does. Therefore, if no color is attached, create a dummy
        // color texture and attach it.
        // This dummy color texture will is owned by the FramebufferObject,
        // and will be released by it when its object is detached.

        GLint type = GL_NONE;
        GLint name = 0;

        ctx->dispatcher().glGetFramebufferAttachmentParameterivEXT(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT_OES, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &type);
        if(type != GL_NONE)
        {
            ctx->dispatcher().glGetFramebufferAttachmentParameterivEXT(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT_OES, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &name);
        }
        else
        {
            ctx->dispatcher().glGetFramebufferAttachmentParameterivEXT(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT_OES, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &type);
            if(type != GL_NONE)
            {
                ctx->dispatcher().glGetFramebufferAttachmentParameterivEXT(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT_OES, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &name);
            }
            else
            {
                // No color, depth or stencil attachments - do nothing
                return;
            }
        }

        // Find the existing attachment(s) dimensions
        GLint width = 0;
        GLint height = 0;

        if(type == GL_RENDERBUFFER)
        {
            GLint prev;
            ctx->dispatcher().glGetIntegerv(GL_RENDERBUFFER_BINDING, &prev);
            ctx->dispatcher().glBindRenderbufferEXT(GL_RENDERBUFFER, name);
            ctx->dispatcher().glGetRenderbufferParameterivEXT(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &width);
            ctx->dispatcher().glGetRenderbufferParameterivEXT(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &height);
            ctx->dispatcher().glBindRenderbufferEXT(GL_RENDERBUFFER, prev);
        }
        else if(type == GL_TEXTURE)
        {
            GLint prev;
            ctx->dispatcher().glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev);
            ctx->dispatcher().glBindTexture(GL_TEXTURE_2D, name);
            ctx->dispatcher().glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
            ctx->dispatcher().glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
            ctx->dispatcher().glBindTexture(GL_TEXTURE_2D, prev);
        }

        // Create the color attachment and attch it
        unsigned int tex = 0;
        ctx->dispatcher().glGenTextures(1, &tex);
        GLint prev;
        ctx->dispatcher().glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev);
        ctx->dispatcher().glBindTexture(GL_TEXTURE_2D, tex);

        ctx->dispatcher().glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
        ctx->dispatcher().glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
        ctx->dispatcher().glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        ctx->dispatcher().glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
        ctx->dispatcher().glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

        ctx->dispatcher().glFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0_OES, GL_TEXTURE_2D, tex, 0);
        setAttachment(GL_COLOR_ATTACHMENT0_OES, GL_TEXTURE_2D, tex, ObjectDataPtr(), true);

        ctx->dispatcher().glBindTexture(GL_TEXTURE_2D, prev);
    }

    if(m_dirty)
    {
        // This is a workaround for a bug found in several OpenGL
        // drivers (e.g. ATI's) - after the framebuffer attachments
        // have changed, and before the next draw, unbind and rebind
        // the framebuffer to sort things out.
        ctx->dispatcher().glBindFramebufferEXT(GL_FRAMEBUFFER,0);
        ctx->dispatcher().glBindFramebufferEXT(
                GL_FRAMEBUFFER,
                ctx->getFBOGlobalName(m_fbName));

        m_dirty = false;
    }
}

void FramebufferData::setDrawBuffers(GLsizei n, const GLenum * bufs) {
    m_drawBuffers.resize(n);
    memcpy(m_drawBuffers.data(), bufs, n * sizeof(GLenum));
    m_hasDrawBuffers = true;
}

void FramebufferData::setReadBuffers(GLenum src) {
    m_readBuffer = src;
}
