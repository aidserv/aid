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
#ifndef GRAPHICS_TRANSLATION_GLES_RENDERBUFFER_DATA_H_
#define GRAPHICS_TRANSLATION_GLES_RENDERBUFFER_DATA_H_

#include <GLES/gl.h>

#include "gles/object_data.h"
#include "gles/egl_image.h"

class RenderbufferData : public ObjectData {
 public:
  explicit RenderbufferData(ObjectLocalName name);

  void SetEglImage(const EglImagePtr& image);
  void SetDataStore(GLenum target, GLenum format, GLint width, GLint height);

  GLuint GetWidth() const;
  GLuint GetHeight() const;
  GLenum GetFormat() const;
  GLint GetEglImageTexture() const;

  void AttachFramebuffer(GLuint name, GLenum target);
  void DetachFramebuffer();

  bool IsAttached() const { return attach_fbo_ != 0; }
  GLenum GetAttachment() const { return attach_point_; }
  GLuint GetAttachedFramebuffer() const { return attach_fbo_; }

 protected:
  virtual ~RenderbufferData();

 private:
  GLuint attach_fbo_;
  GLenum attach_point_;

  GLenum target_;
  GLenum format_;
  GLuint width_;
  GLuint height_;
  EglImagePtr image_;

  RenderbufferData(const RenderbufferData&);
  RenderbufferData& operator=(const RenderbufferData&);
};

typedef android::sp<RenderbufferData> RenderbufferDataPtr;

#endif  // GRAPHICS_TRANSLATION_GLES_RENDERBUFFER_DATA_H_
