/*
 * Copyright (C) 2014 The Android Open Source Project
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

#include "gles/program_variant.h"

#include <algorithm>
#include <stdio.h>

#include "common/alog.h"
#include "gles/gles_context.h"
#include "gles/macros.h"

using android::base::AutoLock;

static GlesContext* GetRequiredContext() {
  GlesContext* ctx = GetCurrentGlesContext();
  LOG_ALWAYS_FATAL_IF(!ctx);
  return ctx;
}

// Maximum buffer length for active attribute and uniform names.
static const int kMaxAttribUniformNameLength = 1024;

///////////////////////////////////////////////////////////////////////////////
// ProgramVariant Main Code
///////////////////////////////////////////////////////////////////////////////

ProgramVariant::ProgramVariant()
    : global_name_(0), is_linked_(false), is_link_attempted_(false) {
}

ProgramVariant::~ProgramVariant() {
  if (global_name_) {
    GlesContext* ctx = GetRequiredContext();
    PASS_THROUGH(ctx, DeleteProgram, global_name_);
  }
}

void ProgramVariant::EnsureLiveProgramObject(GlesContext* ctx) {
  if (!global_name_) {
    global_name_ = PASS_THROUGH(ctx, CreateProgram);
    LOG_ALWAYS_FATAL_IF(!global_name_);
  }
}

GLuint ProgramVariant::GetOrCreateGlobalName() {
  EnsureLiveProgramObject(GetRequiredContext());
  return global_name_;
}

void ProgramVariant::AttachShader(ShaderVariantPtr shader) {
  LOG_ALWAYS_FATAL_IF(is_linked_);
  if (shader->GetObjectType() == VERTEX_SHADER) {
    vertex_shader_ = shader;
  } else {
    fragment_shader_ = shader;
  }
}

void ProgramVariant::DetachShader(ShaderVariantPtr shader) {
  LOG_ALWAYS_FATAL_IF(is_linked_);
  if (shader->GetObjectType() == VERTEX_SHADER) {
    vertex_shader_ = ShaderVariantPtr(NULL);
  } else {
    fragment_shader_ = ShaderVariantPtr(NULL);
  }
}

bool ProgramVariant::UsesExternalTextureAs2D() const {
  return (fragment_shader_ != NULL ?
      fragment_shader_->HasReplacedExternalTextureWith2D() : false);
}

std::string ProgramVariant::GetInfoLog() const {
  AutoLock lock(lock_);
  if (!global_name_ || !info_log_cache_.IsDirty()) {
    return info_log_cache_.Get();
  }

  GLint info_log_length = 0;
  GlesContext* ctx = GetRequiredContext();
  PASS_THROUGH(ctx, GetProgramiv, global_name_,
               GL_INFO_LOG_LENGTH, &info_log_length);
  if (info_log_length <= 0) {
    return "";
  }

  char* buf = new char[info_log_length];
  PASS_THROUGH(ctx, GetProgramInfoLog, global_name_,
               info_log_length, NULL, buf);
  info_log_cache_.Mutate() = buf;
  info_log_cache_.Clean();
  delete[] buf;
  return info_log_cache_.Get();
}

void ProgramVariant::ResetInfoLogCache() {
  info_log_cache_.Mutate() = "";
}

ProgramVariant* ProgramVariant::Clone() const {
  LOG_ALWAYS_FATAL_IF(!is_linked_);
  ProgramVariant* result = new ProgramVariant();
  result->AttachShader(vertex_shader_);
  result->AttachShader(fragment_shader_);
  result->active_attributes_ = active_attributes_;
  result->active_uniforms_ = active_uniforms_;
  if (!info_log_cache_.IsDirty()) {
    result->info_log_cache_.Mutate() = info_log_cache_.Get();
    result->info_log_cache_.Clean();
  }
  // Do not copy uniform locations as they are only known
  // after successful linking.
  return result;
}

void ProgramVariant::Link() {
  LOG_ALWAYS_FATAL_IF(is_linked_);

  is_link_attempted_ = true;

  if (vertex_shader_ == NULL || fragment_shader_ == NULL) {
    info_log_cache_.Mutate() = "Both shaders must be attached";
    info_log_cache_.Clean();
    return;
  }

  if (!vertex_shader_->IsCompileRequested() ||
      !fragment_shader_->IsCompileRequested()) {
    info_log_cache_.Mutate() = "Both shaders must be compiled";
    info_log_cache_.Clean();
    return;
  }

  GlesContext* ctx = GetRequiredContext();
  EnsureLiveProgramObject(ctx);
  PASS_THROUGH(ctx, AttachShader, global_name_,
               vertex_shader_->GetOrCreateGlobalName());
  PASS_THROUGH(ctx, AttachShader, global_name_,
               fragment_shader_->GetOrCreateGlobalName());

  PASS_THROUGH(ctx, LinkProgram, global_name_);

  GLint linkStatus = 0;
  PASS_THROUGH(ctx, GetProgramiv, global_name_, GL_LINK_STATUS, &linkStatus);
  is_linked_ = (linkStatus != GL_FALSE);
  ResetInfoLogCache();

  // Collect information about active attributes and uniforms. A program
  // that failed to link should still try to report active attributes.
  UpdateActiveAttributes();
  UpdateActiveUniforms();
}

///////////////////////////////////////////////////////////////////////////////
// ShaderVariant Active Attributes
///////////////////////////////////////////////////////////////////////////////

void ProgramVariant::UpdateActiveAttributes() {
  active_attributes_.clear();
  attribute_locations_ = requested_attribute_locations_;

  GlesContext* ctx = GetRequiredContext();
  if (ctx->AreChecksEnabled()) {
    int max_needed = 0;
    PASS_THROUGH(ctx, GetProgramiv, global_name_,
                 GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &max_needed);
    LOG_ALWAYS_FATAL_IF(max_needed > kMaxAttribUniformNameLength);
  }

  GLint attribute_count = 0;
  NameLocationMap unknown_attributes(attribute_locations_);
  PASS_THROUGH(ctx, GetProgramiv, global_name_,
               GL_ACTIVE_ATTRIBUTES, &attribute_count);
  for (int i = 0; i < attribute_count; ++i) {
    char name[kMaxAttribUniformNameLength];
    name[0] = 0;
    GLenum type = 0;
    GLint size = 0;
    PASS_THROUGH(ctx, GetActiveAttrib, global_name_, i,
                 sizeof(name), NULL, &size, &type, name);

    if (name[0] == 0 || (strlen(name) > 3 && !strncmp(name, "gl_", 3))) {
      // Either error or a built-in attribute. Neither of those cases needs
      // reporting to the application.
      continue;
    }

    unknown_attributes.erase(name);
    active_attributes_.push_back(ActiveAttribute(name, type, size));
  }

  if (is_linked_) {
    // Remove any unmatched attribute names from the location cache
    // once the program has finally linked.
    for (NameLocationMap::iterator it = unknown_attributes.begin();
          it != unknown_attributes.end(); ++it) {
      ALOGW("Unknown attribute name bound to a shader program: '%s'",
            it->first.c_str());
      attribute_locations_.erase(it->first);
    }
  }
}

int ProgramVariant::GetActiveAttribMaxNameLengh() const {
  size_t max_len = 0;
  for (size_t i = 0; i < active_attributes_.size(); ++i) {
    max_len = std::max(max_len, active_attributes_[i].GetName().size());
  }
  return max_len;
}

void ProgramVariant::GetActiveAttrib(GLuint index, GLsizei buf_size,
                                     GLsizei* length, GLint* size,
                                     GLenum* type, GLchar* name) const {
  if (buf_size < 0) {
    GLES_ERROR_INVALID_VALUE_INT(buf_size);
    return;
  }
  if (index >= active_attributes_.size()) {
    GLES_ERROR_INVALID_VALUE_INT(index);
    return;
  }

  const ActiveAttribute& attribute = active_attributes_[index];
  if (length) {
    *length = attribute.GetName().size();
  }
  *size = attribute.GetSize();
  *type = attribute.GetType();
  snprintf(name, buf_size, "%s", attribute.GetName().c_str());
}

void ProgramVariant::BindAttribLocation(const std::string& name,
                                        GLint location) {
  LOG_ALWAYS_FATAL_IF(is_linked_);
  GlesContext* ctx = GetRequiredContext();
  GLint global_name = GetOrCreateGlobalName();
  PASS_THROUGH(ctx, BindAttribLocation, global_name, location, name.c_str());
  requested_attribute_locations_[name] = location;
}

std::string ProgramVariant::GetActiveAttribNameAndLocation(
    GLuint index, GLint* location) {
  LOG_ALWAYS_FATAL_IF(!is_linked_);
  LOG_ALWAYS_FATAL_IF(index >= active_attributes_.size());
  const std::string& name = active_attributes_[index].GetName();
  *location = GetAttribLocation(name);
  return name;
}

GLint ProgramVariant::GetAttribLocation(const std::string& name) {
  AutoLock lock(lock_);
  LOG_ALWAYS_FATAL_IF(!is_linked_);

  if (attribute_locations_.count(name)) {
    return attribute_locations_[name];
  }

  GlesContext* ctx = GetRequiredContext();
  GLint location = PASS_THROUGH(ctx, GetAttribLocation, global_name_,
                                name.c_str());
  attribute_locations_[name] = location;
  return location;
}

///////////////////////////////////////////////////////////////////////////////
// ShaderVariant Active Uniforms
///////////////////////////////////////////////////////////////////////////////

void ProgramVariant::UpdateActiveUniforms() {
  LOG_ALWAYS_FATAL_IF(!uniform_locations_.empty());

  active_uniforms_.clear();
  uniform_base_names_.clear();

  GlesContext* ctx = GetRequiredContext();
  if (ctx->AreChecksEnabled()) {
    int max_needed = 0;
    PASS_THROUGH(ctx, GetProgramiv, global_name_,
                 GL_ACTIVE_UNIFORM_MAX_LENGTH, &max_needed);
    LOG_ALWAYS_FATAL_IF(max_needed > kMaxAttribUniformNameLength);
  }

  GLint uniform_count = 0;
  PASS_THROUGH(ctx, GetProgramiv, global_name_,
               GL_ACTIVE_UNIFORMS, &uniform_count);
  for (int i = 0; i < uniform_count; ++i) {
    // Each active uniform represents a separate uniform, an array, or
    // an individual element of a struct. Size is only used with arrays.
    char name[kMaxAttribUniformNameLength];
    name[0] = 0;
    GLenum type = 0;
    GLint size = 0;
    PASS_THROUGH(ctx, GetActiveUniform, global_name_, i,
                 sizeof(name), NULL, &size, &type, name);

    int len = strlen(name);
    if (!len || (len > 3 && !strncmp(name, "gl_", 3))) {
      // Either error or a built-in uniform. Neither of those cases can have
      // a location or need reporting to the application.
      continue;
    }

    ALOGV("Found active uniform name '%s', type=0x%x, size=%d",
          name, type, size);

    bool is_array = false;
    std::string base_name = name;
    if (len >= 4 && name[len - 3] == '[' &&
        name[len - 2] == '0' && name[len - 1] == ']' ) {
      // Cut the trailing "[0]" that indicates an array.
      // Per GLES2: If the active uniform is an array, the uniform name
      // returned in name will always be the name of the uniform array
      // appended with "[0]".
      base_name = base_name.substr(0, len - 3);
      is_array = true;
    }

    if (uniform_base_names_.count(base_name)) {
      ALOGW("Detected multiple active uniforms with the same base name: %s",
            name);
    }

    uniform_base_names_[base_name] = active_uniforms_.size();
    active_uniforms_.push_back(
        ActiveUniform(name, base_name, type, is_array, size));
  }
}

int ProgramVariant::GetActiveUniformMaxNameLengh() const {
  size_t max_len = 0;
  for (size_t i = 0; i < active_uniforms_.size(); ++i) {
    max_len = std::max(max_len, active_uniforms_[i].GetOriginalName().size());
  }
  return max_len;
}

const ProgramVariant::ActiveUniform& ProgramVariant::GetActiveUniform(
    GLuint index) const {
  LOG_ALWAYS_FATAL_IF(index >= active_uniforms_.size());
  return active_uniforms_[index];
}

void ProgramVariant::GetActiveUniform(GLuint index, GLsizei buf_size,
                                      GLsizei* length, GLint* size,
                                      GLenum* type, GLchar* name) const {
  if (buf_size < 0) {
    GLES_ERROR_INVALID_VALUE_INT(buf_size);
    return;
  }
  if (index >= active_uniforms_.size()) {
    GLES_ERROR_INVALID_VALUE_INT(index);
    return;
  }

  const ActiveUniform& uniform = active_uniforms_[index];
  if (length) {
    *length = uniform.GetOriginalName().size();
  }
  *size = uniform.GetArraySize();
  *type = uniform.GetType();
  snprintf(name, buf_size, "%s", uniform.GetOriginalName().c_str());
}

std::string ProgramVariant::ParseUniformName(
    const std::string& name, int* array_index) const {
  std::string base_name = name;
  size_t len = name.size();
  if (len > 0 && name[len - 1] == ']') {
    if (len < 4) {
      ALOGW("Invalid uniform name: '%s'", name.c_str());
      return "";  // No space for the name and index.
    }
    size_t pos = name.rfind('[', len - 1);
    if (pos == std::string::npos || pos == 0 || pos == (len - 2)) {
      ALOGW("Invalid uniform name: '%s'", name.c_str());
      return "";  // No square bracket, array name, or index.
    }
    base_name = name.substr(0, pos);
    std::string idx_str = name.substr(pos + 1, len - pos - 2);
    char* endptr = NULL;
    int idx = static_cast<int>(strtol(idx_str.c_str(), &endptr, 10));
    if (idx < 0 || *endptr != '\0') {
      ALOGW("Invalid array index in uniform name: '%s'", name.c_str());
      return "";  // Out-of-range or unparseable index.
    }
    *array_index = idx;
  } else {
    *array_index = 0;
  }

  NameLocationMap::const_iterator it = uniform_base_names_.find(base_name);
  if (it == uniform_base_names_.end()) {
    ALOGI("Unable to find active uniform for '%s' in global program %d, "
          "base name '%s', array index %d",
          name.c_str(), global_name_, base_name.c_str(), *array_index);
    return "";
  }

  GLint array_size = active_uniforms_[it->second].GetArraySize();
  if (*array_index >= array_size) {
    ALOGW("Uniform array index %d out of range (%d): '%s'",
          *array_index, array_size, name.c_str());
    return "";
  }
  return base_name;
}

GLint ProgramVariant::GetUniformLocation(const std::string& name) {
  AutoLock lock(lock_);
  LOG_ALWAYS_FATAL_IF(!is_linked_);

  if (uniform_locations_.count(name)) {
    return uniform_locations_.find(name)->second;
  }

  GlesContext* ctx = GetRequiredContext();
  GLint location = PASS_THROUGH(ctx, GetUniformLocation,
                                global_name_, name.c_str());
  if (location == -1) {
    ALOGW("Uniform '%s' not found in a linked program", name.c_str());
    // Fall through to cache the error result.
  }

  uniform_locations_[name] = location;
  return location;
}
