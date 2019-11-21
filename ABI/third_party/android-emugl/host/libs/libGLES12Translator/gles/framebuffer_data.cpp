/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "gles/framebuffer_data.h"

#include <GLES2/gl2.h>

FramebufferData::Attachment::Attachment() {
  Set(0, 0, RenderbufferDataPtr());
}

void FramebufferData::Attachment::Set(GLenum target,
                                      GLuint name,
                                      const RenderbufferDataPtr& obj) {
  target_ = target;
  name_ = name;
  obj_ = obj;
}

void FramebufferData::Attachment::Detach() {
  if (obj_ != NULL && target_ == GL_RENDERBUFFER) {
    obj_->DetachFramebuffer();
  }
  Set(0, 0, RenderbufferDataPtr());
}

FramebufferData::FramebufferData(ObjectLocalName name)
  : ObjectData(FRAMEBUFFER, name) {
}

FramebufferData::~FramebufferData() {
  for (int i = 0; i < MAX_ATTACH_POINTS; ++i) {
    attachment_[i].Detach();
  }
}

void FramebufferData::SetAttachment(GLenum attachment, GLenum target,
                                    GLuint name,
                                    const RenderbufferDataPtr& obj) {
  const int idx = GetAttachmentPointIndex(attachment);
  if (attachment_[idx].target_ != target ||
      attachment_[idx].name_ != name ||
      attachment_[idx].obj_ != obj) {
    attachment_[idx].Detach();
    attachment_[idx].Set(target, name, obj);

    if (target == GL_RENDERBUFFER && obj != NULL) {
      obj->AttachFramebuffer(GetLocalName(), attachment);
    }
  }
}

GLuint FramebufferData::GetAttachment(GLenum attachment,
                                      GLenum* out_target) const {
  const int idx = GetAttachmentPointIndex(attachment);
  if (out_target) {
    *out_target = attachment_[idx].target_;
  }
  return attachment_[idx].name_;
}

int FramebufferData::GetAttachmentPointIndex(GLenum attachment) const {
  switch (attachment) {
    case GL_COLOR_ATTACHMENT0:
      return ATTACH_POINT_COLOR;
    case GL_DEPTH_ATTACHMENT:
      return ATTACH_POINT_DEPTH;
    case GL_STENCIL_ATTACHMENT:
      return ATTACH_POINT_STENCIL;
    default:
      return MAX_ATTACH_POINTS;
  }
}

void FramebufferData::ClearAttachment(GLenum attachment) {
  const int idx = GetAttachmentPointIndex(attachment);
  attachment_[idx].Detach();
}

void FramebufferData::ClearAttachment(GLuint name, bool clear_texture) {
  for (int idx = 0; idx < MAX_ATTACH_POINTS; ++idx) {
    if (attachment_[idx].name_ == name) {
      const bool is_renderbuffer = (attachment_[idx].obj_ != NULL);
      if (clear_texture != is_renderbuffer) {
        attachment_[idx].Detach();
      }
    }
  }
}
