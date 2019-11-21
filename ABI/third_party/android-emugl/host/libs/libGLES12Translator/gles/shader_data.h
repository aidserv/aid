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
#ifndef GRAPHICS_TRANSLATION_GLES_SHADER_DATA_H_
#define GRAPHICS_TRANSLATION_GLES_SHADER_DATA_H_

#include <GLES/gl.h>
#include <string>

#include "gles/object_data.h"
#include "gles/shader_variant.h"

class ShaderData : public ObjectData {
 public:
  ShaderData(ObjectType type, ObjectLocalName name);

  ShaderVariantPtr GetShaderVariant() const { return shader_variant_; }

  void SetSource(GLsizei count, const GLchar* const* strs, const GLint* len);

  void GetShaderiv(GLenum pname, GLint* params) const;
  void GetInfoLog(GLsizei max_length, GLsizei* length, GLchar* info_log) const;
  void GetSource(GLsizei buf_size, GLsizei* length, GLchar* sourcs) const;
  void SetBinary(GLenum binaryformat, const void *binary, GLsizei length);

  void Compile();

 protected:
  virtual ~ShaderData();

 private:
  ShaderVariantPtr shader_variant_;

  ShaderData(const ShaderData&);
  ShaderData& operator=(const ShaderData&);
};

typedef android::sp<ShaderData> ShaderDataPtr;

#endif  // GRAPHICS_TRANSLATION_GLES_SHADER_DATA_H_
