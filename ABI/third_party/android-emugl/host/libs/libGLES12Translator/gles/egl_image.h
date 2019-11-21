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
#ifndef GRAPHICS_TRANSLATION_GLES_EGL_IMAGE_H_
#define GRAPHICS_TRANSLATION_GLES_EGL_IMAGE_H_

#include <GLES/gl.h>
#include <utils/RefBase.h>

class EglImage;
typedef android::sp<EglImage> EglImagePtr;

class TextureData;
typedef android::sp<TextureData> TextureDataPtr;

class EglImage : public android::RefBase {
 public:
  static EglImagePtr Create(GLenum target, GLuint texture);

  const GLuint width;
  const GLuint height;
  const GLuint format;
  GLenum global_texture_target;
  GLuint global_texture_name;

 private:
  EglImage(GLenum global_texture_target, GLuint global_texture_name,
           const TextureDataPtr& texture);

  EglImage(const EglImage&);
  EglImage& operator=(const EglImage&);
};

#endif  // GRAPHICS_TRANSLATION_GLES_EGL_IMAGE_H_
