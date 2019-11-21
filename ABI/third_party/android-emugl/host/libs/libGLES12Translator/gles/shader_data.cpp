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
#include "gles/shader_data.h"

#include <stdio.h>
#include <GLES2/gl2.h>

#include "common/alog.h"
#include "gles/debug.h"
#include "gles/gles_context.h"
#include "gles/macros.h"

ShaderData::ShaderData(ObjectType type, ObjectLocalName name)
  : ObjectData(type, name), shader_variant_(new ShaderVariant(type)) {
  LOG_ALWAYS_FATAL_IF(type != VERTEX_SHADER && type != FRAGMENT_SHADER);
}

ShaderData::~ShaderData() {
}

void ShaderData::SetSource(GLsizei count,
                           const GLchar* const* strings,
                           const GLint* length) {
  if (shader_variant_->IsCompileRequested()) {
    // Current shader variant has been compiled and cannot be changed, it could
    // be already in use with a program. Avoid touching the current shader
    // variant and create a new one.
    shader_variant_ = ShaderVariantPtr(new ShaderVariant(GetDataType()));
  }

  size_t size = 1;  // +1 for the NULL terminator.
  for (int i = 0; i < count; ++i) {
    if (length && length[i] >= 0) {
      size += length[i];
    } else {
      size += strlen(strings[i]);
    }
  }

  std::string src;
  src.reserve(size);
  for (int i = 0; i < count; i++) {
    if (length && length[i] >= 0) {
      src.append(strings[i], length[i]);
    } else {
      src.append(strings[i]);
    }
  }

  shader_variant_->SetSource(src);

#ifdef ENABLE_API_LOGGING
  ALOGI("Setting shader source for %d(%d):\n"
         "--- Original Source ---\n%s\n"
         "--- Modified Source ---\n%s\n"
         "---  End of Source  ---\n",
         GetLocalName(), shader_variant_->GetOrCreateGlobalName(),
         src.c_str(), shader_variant_->GetUpdatedSource().c_str());
#endif
}

void ShaderData::GetSource(GLsizei buf_size, GLsizei* length,
                           GLchar* source) const {
  int actual_len = 0;
  if (source && buf_size > 0) {
    actual_len = snprintf(source, buf_size, "%s",
                          shader_variant_->GetOriginalSource().c_str());
    if (actual_len >= buf_size) {
      actual_len = buf_size - 1;  // Text was truncated.
    }
  }
  if (length) {
    *length = actual_len;
  }
}

void ShaderData::SetBinary(GLenum binaryformat, const void* binary,
                           GLsizei length) {
  GLES_ERROR(GL_INVALID_OPERATION, "glShaderBinary is not supported");
}

void ShaderData::GetShaderiv(GLenum pname, GLint* params) const {
  switch (pname) {
    case GL_SHADER_TYPE:
      *params = (GetDataType() == VERTEX_SHADER ?
          GL_VERTEX_SHADER : GL_FRAGMENT_SHADER);
      break;
    case GL_DELETE_STATUS:
      *params = GL_FALSE;  // Not supported (crbug.com/424353).
      break;
    case GL_SHADER_SOURCE_LENGTH:
      *params = shader_variant_->GetOriginalSource().size() + 1;
      break;
    case GL_INFO_LOG_LENGTH:
      *params = shader_variant_->GetInfoLog().size() + 1;
      break;
    case GL_COMPILE_STATUS:
      *params = (shader_variant_->VerifySuccessfulCompile() ?
          GL_TRUE : GL_FALSE);
      break;
    default:
      GLES_ERROR_INVALID_ENUM(pname);
      break;
  }
}

void ShaderData::GetInfoLog(GLsizei max_length, GLsizei* length,
                            GLchar* info_log) const {
  int info_len = snprintf(info_log, max_length, "%s",
                          shader_variant_->GetInfoLog().c_str());
  if (info_len >= max_length) {
    info_len = max_length - 1;  // Text was truncated.
  }
  if (length) {
    *length = info_len;
  }
}

void ShaderData::Compile() {
  shader_variant_->Compile();

#ifdef ENABLE_API_LOGGING
  if (!shader_variant_->VerifySuccessfulCompile()) {
    ALOGE("Unable to compile shader %d(%d)\n%s", GetLocalName(),
          shader_variant_->GetOrCreateGlobalName(),
          shader_variant_->GetInfoLog().c_str());
  }
#endif
}
