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

#include "gles/program_data.h"

#include <stdio.h>
#include <GLES/glext.h>

#include "common/alog.h"
#include "gles/debug.h"
#include "gles/gles_context.h"
#include "gles/macros.h"
#include "gles/shader_data.h"

using android::base::AutoLock;

static GlesContext* GetRequiredContext() {
  GlesContext* ctx = GetCurrentGlesContext();
  LOG_ALWAYS_FATAL_IF(!ctx);
  return ctx;
}

ProgramData::ProgramData(ObjectLocalName name)
  : ObjectData(PROGRAM, name), current_program_(new ProgramVariant()),
    uniform_location_gen_(0), has_link_failure_(false),
    has_warned_about_sampler_use_(false), use_count_(0),
    uniform_revision_gen_(0), validation_status_(GL_FALSE) {
}

ProgramData::~ProgramData() {
  if (use_count_) {
    ALOGE("ProgramData destroyed while use count is %d", use_count_);
  }
  ClearUniformCacheLocked();
}

void ProgramData::ModifyProgramLocked() {
  if (linked_programs_.empty() ||
      current_program_ != linked_programs_[0].GetProgram()) {
    return;  // Already modifying a new program variant.
  }
  // The program has been linked and may be in use. Create a new one.
  current_program_ = ProgramVariantPtr(
      linked_programs_[0].GetProgram()->Clone());
  // TODO(crbug.com/441928): Support a case where a linked program gets
  // modified, its compiled shader is re-attached, and then shader source
  // is updated.
}

void ProgramData::AttachShader(const ShaderDataPtr& shader) {
  AutoLock lock(lock_);
  // Check type correctness and that there is no existing shader with that type.
  ObjectType type = shader->GetDataType();
  LOG_ALWAYS_FATAL_IF(type != VERTEX_SHADER && type != FRAGMENT_SHADER);
  for (ShaderList::const_iterator it = shaders_.begin();
        it != shaders_.end(); ++it) {
    if ((*it)->GetDataType() == type) {
      GLES_ERROR(GL_INVALID_OPERATION,
                 "Shader of type '%s' has already been attached",
                 GetEnumString(type));
      return;
    }
  }

  // Add shader to the list and to the currently-built program variant.
  ModifyProgramLocked();
  shaders_.push_back(shader);
  current_program_->AttachShader(shader->GetShaderVariant());
}

void ProgramData::DetachShader(const ShaderDataPtr& shader) {
  AutoLock lock(lock_);
  bool found = false;
  for (ShaderList::iterator it = shaders_.begin();
      it != shaders_.end(); ++it) {
    if (shader == *it) {
      shaders_.erase(it);
      found = true;
      break;
    }
  }
  if (!found) {
    GLES_ERROR(GL_INVALID_OPERATION, "The shader has not been attached");
    return;
  }
  ModifyProgramLocked();
  current_program_->DetachShader(shader->GetShaderVariant());
}

void ProgramData::GetAttachedShaders(GLsizei max_count, GLsizei* count,
                                     GLuint* shaders) const {
  int i = 0;
  for (ShaderList::const_iterator it = shaders_.begin();
        it != shaders_.end() && i < max_count; ++it) {
    shaders[i] = (*it)->GetLocalName();
    ++i;
  }
  if (count) {
    *count = i;
  }
}

ProgramVariantPtr ProgramData::GetProgramWithLinkAttemptLocked() const {
  if (current_program_->IsLinkAttempted()) {
    return current_program_;
  }
  if (!linked_programs_.empty()) {
    return linked_programs_[0].GetProgram();
  }
  return ProgramVariantPtr();
}

void ProgramData::GetProgramiv(GLenum pname, GLint* params) const {
  AutoLock lock(lock_);
  switch (pname) {
    case GL_DELETE_STATUS:
      *params = GL_FALSE;  // Not supported (crbug.com/424353).
      break;
    case GL_INFO_LOG_LENGTH:
      *params = current_program_->GetInfoLog().size() + 1;
      break;
    case GL_ATTACHED_SHADERS:
      *params = shaders_.size();
      break;
    case GL_LINK_STATUS:
      *params = (!linked_programs_.empty() && !has_link_failure_ ?
          GL_TRUE : GL_FALSE);
      break;
    case GL_VALIDATE_STATUS:
      *params = validation_status_;
      break;
    case GL_ACTIVE_ATTRIBUTES: {
      ProgramVariantPtr program = GetProgramWithLinkAttemptLocked();
      if (program != NULL) {
        *params = program->GetActiveAttribCount();
      } else {
        *params = 0;
      }
      break;
    }
    case GL_ACTIVE_ATTRIBUTE_MAX_LENGTH: {
      ProgramVariantPtr program = GetProgramWithLinkAttemptLocked();
      if (program != NULL) {
        *params = program->GetActiveAttribMaxNameLengh() + 1;
      } else {
        *params = 0;
      }
      break;
    }
    case GL_ACTIVE_UNIFORMS: {
      ProgramVariantPtr program = GetProgramWithLinkAttemptLocked();
      if (program != NULL) {
        *params = program->GetActiveUniformCount();
      } else {
        *params = 0;
      }
      break;
    }
    case GL_ACTIVE_UNIFORM_MAX_LENGTH: {
      ProgramVariantPtr program = GetProgramWithLinkAttemptLocked();
      if (program != NULL) {
        *params = program->GetActiveUniformMaxNameLengh() + 1;
      } else {
        *params = 0;
      }
      break;
    }
    default:
      GLES_ERROR_INVALID_ENUM(pname);
      break;
  }
}

void ProgramData::GetInfoLog(GLsizei max_length, GLsizei* length,
                             GLchar* info_log) const {
  // Use current program as it either equals to the linked program, or may have
  // more recent data if there were GL calls after linking.
  int info_len = snprintf(info_log, max_length, "%s",
                          current_program_->GetInfoLog().c_str());
  if (info_len >= max_length) {
    info_len = max_length - 1;  // Text was truncated.
  }
  if (length) {
    *length = info_len;
  }
}

void ProgramData::Link() {
  AutoLock lock(lock_);
  if (current_program_->IsLinked()) {
    return;
  }

  current_program_->Link();
  has_warned_about_sampler_use_ = false;
  has_link_failure_ = !current_program_->IsLinked();
  if (!has_link_failure_) {
    SetLinkedProgramLocked(current_program_);
    return;
  }

  ALOGE("Unable to link program %d(%d)\n%s", GetLocalName(),
        current_program_->GetOrCreateGlobalName(),
        current_program_->GetInfoLog().c_str());

  if (use_count_ == 0) {
    SetLinkedProgramLocked(ProgramVariantPtr(NULL));
  }
}

void ProgramData::SetLinkedProgramLocked(ProgramVariantPtr program) {
  // Reset all mappings.
  linked_programs_.clear();
  ClearUniformCacheLocked();

  if (program == NULL) {
    return;  // No linked program exists.
  }

  for (int i = 0; i < program->GetActiveUniformCount(); ++i) {
    const ProgramVariant::ActiveUniform& uniform =
        program->GetActiveUniform(i);
    uniform_cache_[uniform.GetBaseName()] = new CachedUniform(
        uniform.GetType(), uniform.IsArray(), uniform.GetArraySize());
  }

  linked_programs_.push_back(LinkedProgram(program));
}

bool ProgramData::Use(bool enable) {
  AutoLock lock(lock_);
  if (!enable) {
    // May be invoked without current context from context destructor.
    --use_count_;
    if (has_link_failure_ && !linked_programs_.empty() && use_count_ == 0) {
      // glUseProgram() spec says an active in-use program that failed
      // to re-link should keep working until it goes out of use.
      SetLinkedProgramLocked(ProgramVariantPtr(NULL));
    }
    return true;
  }
  if (linked_programs_.empty()) {
    GLES_ERROR(GL_INVALID_OPERATION, "Shader program has not been linked");
    return false;
  }
  GetRequiredContext();
  ++use_count_;
  return true;
}

void ProgramData::PrepareForRendering(bool* uses_external_as_2d) {
  AutoLock lock(lock_);
  LinkedProgram* program = GetLinkedProgramForRenderingLocked();
  *uses_external_as_2d = program->GetProgram()->UsesExternalTextureAs2D();

  GlesContext* ctx = GetRequiredContext();
  GLint global_program_name = program->GetProgram()->GetOrCreateGlobalName();
  PASS_THROUGH(ctx, UseProgram, global_program_name);

  PropagateUniformValuesLocked(program);
}

void ProgramData::CleanupAfterRendering() {
  GlesContext* ctx = GetRequiredContext();
  PASS_THROUGH(ctx, UseProgram, 0);
}

ProgramData::LinkedProgram* ProgramData::GetLinkedProgramForRenderingLocked() {
  LOG_ALWAYS_FATAL_IF(linked_programs_.empty());
  LOG_ALWAYS_FATAL_IF(
      linked_programs_[0].GetGlobalTextureTarget() != GL_TEXTURE_2D);

  if (!linked_programs_[0].GetProgram()->UsesExternalTextureAs2D()) {
    // The original program did not use external sampler, cannot rewrite it.
    return &linked_programs_[0];
  }

  // Look for the uniforms that refer appropriate texture units.
  GLint texture_unit_index = -1;
  for (UniformCacheMap::const_iterator it = uniform_cache_.begin();
        it != uniform_cache_.end(); ++it) {
    const CachedUniform* uniform = it->second;
    if (uniform->GetType() != GL_SAMPLER_2D) {
      continue;
    }
    if (uniform->GetArraySize() != 1) {
      if (!has_warned_about_sampler_use_) {
        ALOGW("Shader program uses a sampler array, which is not supported "
              "for backend texture adjustments (used with HW video)");
        has_warned_about_sampler_use_ = true;
      }
      return &linked_programs_[0];
    }
    if (texture_unit_index != -1) {
      if (!has_warned_about_sampler_use_) {
        ALOGW("Shader program uses more than multiple active 2D samplers, "
              "which is not supported for backend texture adjustments "
              "(used with HW video)");
        has_warned_about_sampler_use_ = true;
      }
      return &linked_programs_[0];
    }
    LOG_ALWAYS_FATAL_IF(!uniform->StoreAsIntTo(&texture_unit_index));
  }

  // Find the global target for the sampler's texture.
  GLenum global_target = GL_TEXTURE_2D;
  if (texture_unit_index != -1) {
    GlesContext* ctx = GetRequiredContext();
    global_target = ctx->texture_context_.GetGlobalTarget(
        GL_TEXTURE0 + texture_unit_index, GL_TEXTURE_EXTERNAL_OES);
  }

  // The default program is always targeting GL_TEXTURE_2D.
  if (global_target == GL_TEXTURE_2D) {
    return &linked_programs_[0];
  }

  // Find or create a program for another texture target.
  for (size_t i = 1; i < linked_programs_.size(); ++i) {
    LinkedProgram* program = &linked_programs_[i];
    if (program->GetGlobalTextureTarget() == global_target) {
      return program;
    }
  }

  if (global_target == GL_TEXTURE_EXTERNAL_OES) {
    LinkedProgram* program = linked_programs_[0].Clone(global_target);
    if (!program) {
      if (!has_warned_about_sampler_use_) {
        ALOGW("Unable to rewrite shader program, which is required for "
              "backend texture adjustments (used with HW video)");
        has_warned_about_sampler_use_ = true;
      }
      return &linked_programs_[0];
    }
    linked_programs_.push_back(*program);
    return &linked_programs_[linked_programs_.size() - 1];
  }

  ALOGE("Shader program is used with unsupported texture target %s(0x%x)",
        GetEnumString(global_target), global_target);
  return &linked_programs_[0];
}

void ProgramData::Validate() {
  AutoLock lock(lock_);
  if (linked_programs_.empty()) {
    return;
  }

  bool has_stale_uniform_data = false;
  LinkedProgram* program = &linked_programs_[0];
  for (UniformCacheMap::const_iterator it = uniform_cache_.begin();
        it != uniform_cache_.end(); ++it) {
    if (it->second->GetRevision() > program->GetUniformRevision()) {
      has_stale_uniform_data = true;
      break;
    }
  }

  GlesContext* ctx = GetRequiredContext();
  GLint program_name = program->GetProgram()->GetOrCreateGlobalName();
  if (has_stale_uniform_data) {
    PASS_THROUGH(ctx, UseProgram, program_name);
    PropagateUniformValuesLocked(program);
    PASS_THROUGH(ctx, UseProgram, 0);
  }

  PASS_THROUGH(ctx, ValidateProgram, program_name);
  PASS_THROUGH(ctx, GetProgramiv, program_name, GL_VALIDATE_STATUS,
               &validation_status_);
  program->GetProgram()->ResetInfoLogCache();
}

///////////////////////////////////////////////////////////////////////////////
// Uniform support.
///////////////////////////////////////////////////////////////////////////////

void ProgramData::GetActiveUniform(GLuint index, GLsizei buf_size,
                                   GLsizei* length, GLint* size, GLenum* type,
                                   GLchar* name) const {
  ProgramVariantPtr program = GetProgramWithLinkAttemptLocked();
  if (program != NULL) {
    program->GetActiveUniform(index, buf_size, length, size, type, name);
    return;
  }

  GLES_ERROR(GL_INVALID_OPERATION, "The program has not been linked");
}

void ProgramData::ClearUniformCacheLocked() {
  for (UniformCacheMap::iterator it = uniform_cache_.begin();
        it != uniform_cache_.end(); ++it) {
    delete it->second;
  }
  uniform_cache_.clear();
  uniform_locations_.clear();
  uniform_revision_gen_ = 0;
}

GLint ProgramData::GetUniformLocation(const GLchar* name) {
  AutoLock lock(lock_);
  if (linked_programs_.empty()) {
    GLES_ERROR(GL_INVALID_OPERATION, "The program has not been linked");
    return -1;
  }

  // Look for known local locations.
  for (UniformLocationMap::const_iterator it = uniform_locations_.begin();
        it != uniform_locations_.end(); ++it) {
    if (it->second.GetOriginalName() == name) {
      return it->first;
    }
  }

  // Look up matching active uniform.
  int array_index = -1;
  ProgramVariantPtr program = linked_programs_[0].GetProgram();
  std::string base_name = program->ParseUniformName(name, &array_index);
  if (base_name.empty()) {
    return -1;
  }

  // A sanity check for the location in the underlying shader program.
  GLint global_location = program->GetUniformLocation(name);
  if (global_location == -1) {
    return -1;
  }

  // Add location to the uniform map.
  GLint local_location = ++uniform_location_gen_;
  uniform_locations_[local_location] = UniformLocation(
      name, base_name, array_index);
  return local_location;
}

void ProgramData::PropagateUniformValuesLocked(LinkedProgram* program) {
  GlesContext* ctx = GetRequiredContext();
  uint64_t max_revision = 0;
  for (UniformCacheMap::const_iterator it = uniform_cache_.begin();
        it != uniform_cache_.end(); ++it) {
    const CachedUniform* uniform = it->second;
    uint64_t revision = uniform->GetRevision();
    if (revision > max_revision) {
      max_revision = revision;
    }
    if (revision <= program->GetUniformRevision()) {
      continue;
    }
    // Lookup location of the base uniform name, then set the entire value.
    // TODO(crbug.com/441930): Propagate only those array elements that were
    // modified.
    GLint global_location =
        program->GetProgram()->GetUniformLocation(it->first);
    if (global_location == -1) {
      ALOGW("Unable to find global location for uniform '%s'",
            it->first.c_str());
      continue;
    }
    uniform->SetOnProgram(ctx, global_location);
  }
  program->SetUniformRevision(max_revision);
}

void ProgramData::SetUniformInternal(GLint location,
                                     const UniformValue& value) {
  AutoLock lock(lock_);
  if (location == -1) {
    return;  // GLES2 requires -1 to be ignored.
  }
  if (!uniform_locations_.count(location)) {
    GLES_ERROR(GL_INVALID_OPERATION,
               "Uniform location unknown or program has not been linked");
    return;
  }

  const UniformLocation& location_info =
      uniform_locations_.find(location)->second;
  UniformCacheMap::iterator it =
      uniform_cache_.find(location_info.GetBaseName());
  LOG_ALWAYS_FATAL_IF(it == uniform_cache_.end());
  CachedUniform* uniform = it->second;

  if (value.IsArray() && !uniform->IsArray()) {
    GLES_ERROR(GL_INVALID_OPERATION,
               "Unable to set uniform with count > 1 on a non-array");
    return;
  }

  for (int i = 0; i < value.GetArraySize(); ++i) {
    int target_array_idx = location_info.GetArrayIndex() + i;
    if (target_array_idx >= uniform->GetArraySize()) {
      break;
    }
    if (!uniform->CopyElementFrom(target_array_idx, value, i)) {
      GLES_ERROR(GL_INVALID_OPERATION, "Unable to convert uniform data");
      return;
    }
  }
  uniform->SetRevision(++uniform_revision_gen_);
}

void ProgramData::Uniformfv(GLint location, GLenum type, GLsizei count,
                             const GLfloat* value) {
  if (count < 0) {
    GLES_ERROR_INVALID_VALUE_INT(count);
    return;
  }
  bool is_array = count > 1;
  SetUniformInternal(location, UniformValue(type, is_array, count, value));
}

void ProgramData::Uniformiv(GLint location, GLenum type, GLsizei count,
                             const GLint* value) {
  if (count < 0) {
    GLES_ERROR_INVALID_VALUE_INT(count);
    return;
  }
  bool is_array = count > 1;
  SetUniformInternal(location, UniformValue(type, is_array, count, value));
}

void ProgramData::UniformMatrixfv(GLint location, GLenum type, GLsizei count,
                                  GLboolean transpose, const GLfloat* value) {
  if (count < 0) {
    GLES_ERROR_INVALID_VALUE_INT(count);
    return;
  }
  if (transpose) {
    GLES_ERROR_INVALID_VALUE_INT(static_cast<int>(transpose));
    return;
  }
  bool is_array = count > 1;
  SetUniformInternal(location, UniformValue(type, is_array, count, value));
}

void ProgramData::GetUniformfv(GLint location, GLfloat* params) const {
  AutoLock lock(lock_);
  if (!uniform_locations_.count(location)) {
    GLES_ERROR(GL_INVALID_OPERATION,
               "Uniform location unknown or program has not been linked");
    return;
  }

  const UniformLocation& location_info =
      uniform_locations_.find(location)->second;
  UniformCacheMap::const_iterator it =
      uniform_cache_.find(location_info.GetBaseName());
  LOG_ALWAYS_FATAL_IF(it == uniform_cache_.end());
  if (!it->second->StoreAsFloatTo(params)) {
    GLES_ERROR(GL_INVALID_OPERATION, "Unable to convert uniform data");
  }
}

void ProgramData::GetUniformiv(GLint location, GLint* params) const {
  AutoLock lock(lock_);
  if (!uniform_locations_.count(location)) {
    GLES_ERROR(GL_INVALID_OPERATION,
               "Uniform location unknown or program has not been linked");
    return;
  }

  const UniformLocation& location_info =
      uniform_locations_.find(location)->second;
  UniformCacheMap::const_iterator it =
      uniform_cache_.find(location_info.GetBaseName());
  LOG_ALWAYS_FATAL_IF(it == uniform_cache_.end());
  if (!it->second->StoreAsIntTo(params)) {
    GLES_ERROR(GL_INVALID_OPERATION, "Unable to convert uniform data");
  }
}

///////////////////////////////////////////////////////////////////////////////
// Attribute support.
///////////////////////////////////////////////////////////////////////////////

void ProgramData::GetActiveAttrib(GLuint index, GLsizei buf_size,
                                  GLsizei* length, GLint* size,
                                  GLenum* type, GLchar* name) const {
  ProgramVariantPtr program = GetProgramWithLinkAttemptLocked();
  if (program != NULL) {
    program->GetActiveAttrib(index, buf_size, length, size, type, name);
    return;
  }

  GLES_ERROR(GL_INVALID_OPERATION, "The program has not been linked");
}

void ProgramData::BindAttribLocation(GLuint index, const GLchar* name) {
  AutoLock lock(lock_);
  // Binding of attributes can only happen before linking.
  ModifyProgramLocked();
  current_program_->BindAttribLocation(name, index);
}

GLint ProgramData::GetAttribLocation(const GLchar* name) {
  AutoLock lock(lock_);
  if (linked_programs_.empty()) {
    GLES_ERROR(GL_INVALID_OPERATION, "The program has not been linked");
    return -1;
  }
  return linked_programs_[0].GetProgram()->GetAttribLocation(name);
}

///////////////////////////////////////////////////////////////////////////////
// CachedUniform
///////////////////////////////////////////////////////////////////////////////

bool ProgramData::CachedUniform::CopyElementFrom(int dst_idx,
                                                 const UniformValue& src,
                                                 int src_idx) {
  return value_.CopyElementFrom(dst_idx, src, src_idx);
}

bool ProgramData::CachedUniform::StoreAsFloatTo(GLfloat* value) const {
  return value_.StoreAsFloatTo(value);
}

bool ProgramData::CachedUniform::StoreAsIntTo(GLint* value) const {
  return value_.StoreAsIntTo(value);
}

void ProgramData::CachedUniform::SetOnProgram(GlesContext* ctx,
                                              GLuint global_location) const {
  value_.SetOnProgram(ctx, global_location);
}

///////////////////////////////////////////////////////////////////////////////
// LinkedProgram
///////////////////////////////////////////////////////////////////////////////

ProgramData::LinkedProgram::LinkedProgram(ProgramVariantPtr program)
    : program_(program), uniform_revision_(0) {
  LOG_ALWAYS_FATAL_IF(program_ == NULL);
  LOG_ALWAYS_FATAL_IF(!program_->IsLinked());
}

ProgramData::LinkedProgram* ProgramData::LinkedProgram::Clone(
    GLenum global_texture_target) const {
  if (!program_->UsesExternalTextureAs2D()) {
    // It is not possible to use other global targets if the user's program
    // does not use external textures.
    return NULL;
  }

  // Create the new program with proper shaders.
  ProgramVariantPtr new_program = ProgramVariantPtr(new ProgramVariant());
  new_program->AttachShader(program_->GetVertexShader());
  ShaderVariantPtr fragment_shader = ShaderVariantPtr(
      new ShaderVariant(FRAGMENT_SHADER));
  fragment_shader->SetGlobalTextureTarget(global_texture_target);
  fragment_shader->SetSource(
      program_->GetFragmentShader()->GetOriginalSource());
  fragment_shader->Compile();
  new_program->AttachShader(fragment_shader);

#ifdef ENABLE_API_LOGGING
  ALOGI("Rewrote fragment shader old_name=%d new_name=%d:\n"
         "--- Original Source ---\n%s\n"
         "--- Modified Source ---\n%s\n"
         "---  End of Source  ---\n",
         program_->GetFragmentShader()->GetOrCreateGlobalName(),
         fragment_shader->GetOrCreateGlobalName(),
         fragment_shader->GetOriginalSource().c_str(),
         fragment_shader->GetUpdatedSource().c_str());
#endif

  // Transfer vertex attribute bindings as they refer to app's GL context.
  for (int i = 0; i < program_->GetActiveAttribCount(); ++i) {
    GLint location = -1;
    std::string name = program_->GetActiveAttribNameAndLocation(i, &location);
    if (location != -1) {
      new_program->BindAttribLocation(name, location);
    }
  }

  // Link the new program.
  new_program->Link();
  if (!new_program->IsLinked()) {
    ALOGW("Unable to link program for texture target 0x%x",
          global_texture_target);
    return NULL;
  }

  return new LinkedProgram(new_program);
}
