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
#ifndef GRAPHICS_TRANSLATION_GLES_PROGRAM_VARIANT_H_
#define GRAPHICS_TRANSLATION_GLES_PROGRAM_VARIANT_H_

#include <GLES/gl.h>
#include <map>
#include <string>
#include <utils/RefBase.h>
#include <vector>

#include "android/base/synchronization/Lock.h"

#include "gles/dirtiable.h"
#include "gles/object_data.h"
#include "gles/shader_variant.h"

class GlesContext;

// Represents a program adjusted for a particular use. Normally, there is only
// one variant per linked program. A new variant may appear when the backing
// texture target requires a different variant of the fragment shader.
// Functions (including destructor) may pass calls to the underlying GL
// implementation, and so must be called with an active context.
// A linked ProgramVariant instance cannot be re-linked.
class ProgramVariant : public android::RefBase {
 public:
  ProgramVariant();

  bool IsLinked() const { return is_linked_; }
  bool IsLinkAttempted() const { return is_link_attempted_; }

  std::string GetInfoLog() const;
  void ResetInfoLogCache();

  GLuint GetOrCreateGlobalName();

  void AttachShader(ShaderVariantPtr shader);
  void DetachShader(ShaderVariantPtr shader);
  ShaderVariantPtr GetVertexShader() const { return vertex_shader_; }
  ShaderVariantPtr GetFragmentShader() const { return fragment_shader_; }

  void Link();

  // Clones this program into a new variant. Current program must be linked.
  // The resulting program will have the same shaders, but will not be linked.
  ProgramVariant* Clone() const;

  bool UsesExternalTextureAs2D() const;

  // Describes one active attribute of this program.
  struct ActiveAttribute {
    ActiveAttribute() : type_(-1), size_(-1) {}
    ActiveAttribute(const std::string& name, GLenum type, GLint size)
        : name_(name), type_(type), size_(size) {}

    const std::string& GetName() const { return name_; }
    GLenum GetType() const { return type_; }
    GLint GetSize() const { return size_; }

   private:
    std::string name_;
    GLenum type_;
    GLint size_;
  };

  int GetActiveAttribCount() const { return active_attributes_.size(); }
  int GetActiveAttribMaxNameLengh() const;
  void GetActiveAttrib(GLuint index, GLsizei buf_size, GLsizei* length,
                       GLint* size, GLenum* type, GLchar* name) const;
  std::string GetActiveAttribNameAndLocation(GLuint index, GLint* location);
  void BindAttribLocation(const std::string& name, GLint location);
  GLint GetAttribLocation(const std::string& name);

  // Describes one active uniform of this program.
  struct ActiveUniform {
    ActiveUniform() : type_(0), is_array_(false), array_size_(0) {}
    ActiveUniform(const std::string& original_name,
                  const std::string& base_name,
                  GLenum type, bool is_array, GLint array_size)
        : original_name_(original_name), base_name_(base_name),
          type_(type), is_array_(is_array), array_size_(array_size) {}

    const std::string& GetOriginalName() const { return original_name_; }
    const std::string& GetBaseName() const { return base_name_; }
    GLenum GetType() const { return type_; }
    bool IsArray() const { return is_array_; }
    GLint GetArraySize() const { return array_size_; }

   private:
    std::string original_name_;
    std::string base_name_;
    GLenum type_;
    bool is_array_;
    GLint array_size_;
  };

  int GetActiveUniformCount() const { return active_uniforms_.size(); }
  int GetActiveUniformMaxNameLengh() const;
  void GetActiveUniform(GLuint index, GLsizei buf_size, GLsizei* length,
                        GLint* size, GLenum* type, GLchar* name) const;
  const ActiveUniform& GetActiveUniform(GLuint index) const;
  // Parses the given name, matches against the list of known active uniforms,
  // verifies array index and returns uniform base name.
  // In case of error returns empty string.
  std::string ParseUniformName(
      const std::string& name, int* array_index) const;
  GLint GetUniformLocation(const std::string& name);

 protected:
  virtual ~ProgramVariant();

 private:
  typedef std::map<std::string, GLint> NameLocationMap;

  void EnsureLiveProgramObject(GlesContext* ctx);
  void UpdateActiveAttributes();
  void UpdateActiveUniforms();

  GLuint global_name_;
  ShaderVariantPtr vertex_shader_;
  ShaderVariantPtr fragment_shader_;
  std::vector<ActiveAttribute> active_attributes_;
  std::vector<ActiveUniform> active_uniforms_;
  // Maps base name to index inside active_uniforms_.
  NameLocationMap uniform_base_names_;
  NameLocationMap attribute_locations_;
  NameLocationMap requested_attribute_locations_;
  NameLocationMap uniform_locations_;
  bool is_linked_;
  bool is_link_attempted_;
  mutable Dirtiable<std::string> info_log_cache_;
  mutable android::base::Lock lock_;

  ProgramVariant(const ProgramVariant&);
  ProgramVariant& operator=(const ProgramVariant&);
};

typedef android::sp<ProgramVariant> ProgramVariantPtr;

#endif  // GRAPHICS_TRANSLATION_GLES_PROGRAM_VARIANT_H_
