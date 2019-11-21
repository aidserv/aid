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
#ifndef GRAPHICS_TRANSLATION_GLES_SHADER_VARIANT_H_
#define GRAPHICS_TRANSLATION_GLES_SHADER_VARIANT_H_

#include <GLES/gl.h>
#include <string>
#include <utils/RefBase.h>

#include "android/base/synchronization/Lock.h"

#include "gles/dirtiable.h"
#include "gles/object_data.h"

class GlesContext;

// Represents a shader adjusted for a particular use. Normally, there is only
// one variant per compiled user-visible shader. A new variant may appear
// when user modifies source of a previously compiled shader, or implicitly
// when the backing texture target requires a different shader program.
// Functions (including destructor) may pass calls to the underlying GL
// implementation, and so must be called with an active context.
// A compiled ShaderVariant instance cannot be re-compiled.
class ShaderVariant : public android::RefBase {
 public:
  explicit ShaderVariant(ObjectType object_type);

  ObjectType GetObjectType() const { return object_type_; }
  bool IsCompileRequested() const {
    return compile_status_ != COMPILE_NOT_ATTEMPTED;
  }

  bool VerifySuccessfulCompile() const;

  std::string GetInfoLog() const;

  GLuint GetOrCreateGlobalName();

  void SetSource(const std::string& str);
  const std::string& GetOriginalSource() const { return original_source_; }
  const std::string& GetUpdatedSource() const { return updated_source_; }

  // Modifies the expected backing texture. Must be called before SetSource().
  void SetGlobalTextureTarget(GLenum target);
  GLenum GetGlobalTextureTarget() const { return global_texture_target_; }

  // Returns true if local texture target used by the original source was
  // external, and the program was modified to use TEXTURE_2D.
  bool HasReplacedExternalTextureWith2D() const {
    return has_replaced_external_texture_with_2d_;
  }

  void Compile();

 protected:
  virtual ~ShaderVariant();

 private:
  enum CompileStatus {
    COMPILE_NOT_ATTEMPTED,
    COMPILE_ATTEMPTED,
    COMPILE_SUCCESSFUL,
    COMPILE_FAILED,
  };

  ObjectType object_type_;
  GLuint global_name_;
  mutable CompileStatus compile_status_;
  mutable Dirtiable<std::string> info_log_cache_;
  std::string original_source_;
  std::string updated_source_;
  GLenum global_texture_target_;
  bool has_replaced_external_texture_with_2d_;
  mutable android::base::Lock lock_;

  void EnsureLiveShaderObject(GlesContext* ctx);
  void RewriteSource();

  static int ExtractVersion(std::string* src);
  static bool Substitute(std::string* src, const char* match,
                         const char* repl);

  ShaderVariant(const ShaderVariant&);
  ShaderVariant& operator=(const ShaderVariant&);
};

typedef android::sp<ShaderVariant> ShaderVariantPtr;

#endif  // GRAPHICS_TRANSLATION_GLES_SHADER_VARIANT_H_
