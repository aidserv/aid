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
#include "gles/renderbuffer_data.h"

RenderbufferData::RenderbufferData(ObjectLocalName name)
  : ObjectData(RENDERBUFFER, name),
    attach_fbo_(0),
    attach_point_(0),
    target_(0),
    format_(0),
    width_(0),
    height_(0) {
}

RenderbufferData::~RenderbufferData() {
}

void RenderbufferData::AttachFramebuffer(GLuint name, GLenum target) {
  attach_fbo_ = name;
  attach_point_ = target;
}

void RenderbufferData::DetachFramebuffer() {
  attach_fbo_ = 0;
  attach_point_ = 0;
}

GLuint RenderbufferData::GetWidth() const {
  return image_ != NULL ? image_->width : width_;
}

GLuint RenderbufferData::GetHeight() const {
  return image_ != NULL ? image_->height : height_;
}

GLenum RenderbufferData::GetFormat() const {
  return image_ != NULL ? image_->format : format_;
}

GLint RenderbufferData::GetEglImageTexture() const {
  return image_ != NULL ? image_->global_texture_name : 0;
}

void RenderbufferData::SetEglImage(const EglImagePtr& image) {
  image_ = image;
  target_ = 0;
  format_ = 0;
  width_ = 0;
  height_ = 0;
}

void RenderbufferData::SetDataStore(GLenum target, GLenum format, GLint width,
                                    GLint height) {
  image_ = NULL;
  target_ = target;
  format_ = format;
  width_ = width;
  height_ = height;
}
