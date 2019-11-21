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
#ifndef GRAPHICS_TRANSLATION_GLES_PROGRAM_DATA_H_
#define GRAPHICS_TRANSLATION_GLES_PROGRAM_DATA_H_

#include <GLES/gl.h>
#include <map>
#include <string>
#include <vector>

#include "android/base/synchronization/Lock.h"

#include "gles/object_data.h"
#include "gles/shader_data.h"
#include "gles/program_variant.h"
#include "gles/uniform_value.h"

class ProgramData : public ObjectData {
 public:
  explicit ProgramData(ObjectLocalName name);

  void AttachShader(const ShaderDataPtr& shader);
  void DetachShader(const ShaderDataPtr& shader);

  void GetAttachedShaders(GLsizei max_count, GLsizei* count,
                          GLuint* shaders) const;
  void GetProgramiv(GLenum pname, GLint* params) const;
  void GetInfoLog(GLsizei max_length, GLsizei* length, GLchar* info_log) const;

  void Link();

  // Sets this program as active or inactive on the current GL context.
  // Returns true on success. Sets GL error on failure.
  bool Use(bool enable);

  // Selects proper program and propagates uniforms for rendering.
  // The program must be linked and currently in use.
  void PrepareForRendering(bool* uses_external_as_2d);
  void CleanupAfterRendering();

  void Validate();

  // Uniform support.
  GLint GetUniformLocation(const GLchar* name);
  void GetActiveUniform(GLuint index, GLsizei buf_size, GLsizei* length,
                        GLint* size, GLenum* type, GLchar* name) const;
  void GetUniformfv(GLint location, GLfloat* params) const;
  void GetUniformiv(GLint location, GLint* params) const;
  void Uniformfv(GLint location, GLenum type, GLsizei count,
                 const GLfloat* value);
  void Uniformiv(GLint location, GLenum type, GLsizei count,
                 const GLint* value);
  void UniformMatrixfv(GLint location, GLenum type, GLsizei count,
                       GLboolean transpose, const GLfloat* value);

  // Attribute support.
  // We only perform housekeeping for attrib-index bindings and not values,
  // as the actual vertex arrays are a part of context state.
  void GetActiveAttrib(GLuint index, GLsizei buf_size, GLsizei* length,
                       GLint* size, GLenum* type, GLchar* name) const;
  void BindAttribLocation(GLuint index, const GLchar* name);
  GLint GetAttribLocation(const GLchar* name);

 protected:
  virtual ~ProgramData();

 private:
  // Stores a cached uniform value.
  struct CachedUniform {
    CachedUniform(GLenum type, bool is_array, GLint array_size)
        : value_(type, is_array, array_size), revision_(0) {}

    GLenum GetType() const { return value_.GetType(); }
    bool IsArray() const { return value_.IsArray(); }
    GLint GetArraySize() const { return value_.GetArraySize(); }

    bool CopyElementFrom(int dst_idx, const UniformValue& src, int src_idx);
    bool StoreAsFloatTo(GLfloat* value) const;
    bool StoreAsIntTo(GLint* value) const;

    // Sets uniform's value at the given location on the current program.
    void SetOnProgram(GlesContext* ctx, GLuint global_location) const;

    uint64_t GetRevision() const { return revision_; }
    void SetRevision(uint64_t value) { revision_ = value; }

   private:
    UniformValue value_;
    uint64_t revision_;

    CachedUniform(const CachedUniform& src);
    CachedUniform& operator=(const CachedUniform& src);
  };

  // Contains a reference to an active uniform.
  struct UniformLocation {
    UniformLocation() : array_index_(-1) {}
    UniformLocation(const std::string& original_name,
                    const std::string& base_name, int array_index)
        : original_name_(original_name), base_name_(base_name),
          array_index_(array_index) {}

    const std::string& GetOriginalName() const { return original_name_; }
    const std::string& GetBaseName() const { return base_name_; }
    int GetArrayIndex() const { return array_index_; }

   private:
    std::string original_name_;
    std::string base_name_;
    int array_index_;
  };

  // Represents a variant of a linked program.
  struct LinkedProgram {
    LinkedProgram() : program_(NULL), uniform_revision_(0) {}
    explicit LinkedProgram(ProgramVariantPtr program);

    ProgramVariantPtr GetProgram() const { return program_; }

    GLenum GetGlobalTextureTarget() const {
      return program_->GetFragmentShader()->GetGlobalTextureTarget();
    }

    uint64_t GetUniformRevision() const { return uniform_revision_; }
    void SetUniformRevision(uint64_t value) { uniform_revision_ = value; }

    LinkedProgram* Clone(GLenum global_texture_target) const;

   private:
    ProgramVariantPtr program_;
    uint64_t uniform_revision_;
  };

  typedef std::map<GLint, UniformLocation> UniformLocationMap;
  typedef std::map<std::string, CachedUniform*> UniformCacheMap;
  typedef std::vector<ShaderDataPtr> ShaderList;

  void ModifyProgramLocked();
  void SetLinkedProgramLocked(ProgramVariantPtr program);
  LinkedProgram* GetLinkedProgramForRenderingLocked();

  // Returns current program once a Link() has been attempted. Otherwise
  // returns first linked ProgramVariant. If none, returns NULL.
  ProgramVariantPtr GetProgramWithLinkAttemptLocked() const;

  void ClearUniformCacheLocked();
  void SetUniformInternal(GLint location, const UniformValue& value);
  void PropagateUniformValuesLocked(LinkedProgram* program);

  ProgramVariantPtr current_program_;
  std::vector<LinkedProgram> linked_programs_;
  ShaderList shaders_;
  int uniform_location_gen_;
  bool has_link_failure_;
  bool has_warned_about_sampler_use_;
  int use_count_;
  // Values for every active uniform, by base name.
  UniformCacheMap uniform_cache_;
  // Local locations as reported to the app.
  UniformLocationMap uniform_locations_;
  uint64_t uniform_revision_gen_;
  GLint validation_status_;
  mutable android::base::Lock lock_;

  ProgramData(const ProgramData&);
  ProgramData& operator=(const ProgramData&);
};

typedef android::sp<ProgramData> ProgramDataPtr;

#endif  // GRAPHICS_TRANSLATION_GLES_PROGRAM_DATA_H_
