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
#ifndef GRAPHICS_TRANSLATION_GLES_FRAMEBUFFER_DATA_H_
#define GRAPHICS_TRANSLATION_GLES_FRAMEBUFFER_DATA_H_

#include <GLES/gl.h>

#include "gles/object_data.h"
#include "gles/renderbuffer_data.h"

class FramebufferData : public ObjectData {
 public:
  explicit FramebufferData(ObjectLocalName name);

  GLuint GetAttachment(GLenum attachment, GLenum* out_target) const;

  void SetAttachment(GLenum attachment, GLenum target, GLuint name,
                     const RenderbufferDataPtr& obj);

  void ClearAttachment(GLenum attachment);
  void ClearAttachment(GLuint name, bool clear_texture);

 protected:
  virtual ~FramebufferData();

 private:
  enum AttachPoint {
    ATTACH_POINT_COLOR,
    ATTACH_POINT_DEPTH,
    ATTACH_POINT_STENCIL,
    MAX_ATTACH_POINTS,
  };

  struct Attachment {
    Attachment();
    void Set(GLenum target, GLuint name, const RenderbufferDataPtr& obj);
    void Detach();
    GLenum target_;
    GLuint name_;
    RenderbufferDataPtr obj_;
  };

  int GetAttachmentPointIndex(GLenum attachment) const;

  Attachment attachment_[MAX_ATTACH_POINTS];

  FramebufferData(const FramebufferData&);
  FramebufferData& operator=(const FramebufferData&);
};

typedef android::sp<FramebufferData> FramebufferDataPtr;

#endif  // GRAPHICS_TRANSLATION_GLES_FRAMEBUFFER_DATA_H_
