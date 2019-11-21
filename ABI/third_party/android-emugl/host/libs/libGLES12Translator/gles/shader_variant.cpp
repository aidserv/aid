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

#include "gles/shader_variant.h"

#include <stdio.h>
#include <string.h>
#include <algorithm>
#include "common/alog.h"
#include "gles/gles_context.h"
#include "gles/gles_validate.h"
#include "gles/macros.h"

using android::base::AutoLock;

static GlesContext* GetRequiredContext() {
  GlesContext* ctx = GetCurrentGlesContext();
  LOG_ALWAYS_FATAL_IF(!ctx);
  return ctx;
}

ShaderVariant::ShaderVariant(ObjectType object_type)
    : object_type_(object_type), global_name_(0),
      compile_status_(COMPILE_NOT_ATTEMPTED),
      global_texture_target_(GL_TEXTURE_2D),
      has_replaced_external_texture_with_2d_(false) {
  LOG_ALWAYS_FATAL_IF(object_type != VERTEX_SHADER &&
                      object_type != FRAGMENT_SHADER);
}

ShaderVariant::~ShaderVariant() {
  if (global_name_) {
    GlesContext* ctx = GetRequiredContext();
    PASS_THROUGH(ctx, DeleteShader, global_name_);
  }
}

void ShaderVariant::EnsureLiveShaderObject(GlesContext* ctx) {
  if (!global_name_) {
    if (object_type_ == VERTEX_SHADER) {
      global_name_ = PASS_THROUGH(ctx, CreateShader, GL_VERTEX_SHADER);
    } else {
      global_name_ = PASS_THROUGH(ctx, CreateShader, GL_FRAGMENT_SHADER);
    }
    LOG_ALWAYS_FATAL_IF(!global_name_);
  }
}

GLuint ShaderVariant::GetOrCreateGlobalName() {
  EnsureLiveShaderObject(GetRequiredContext());
  return global_name_;
}

void ShaderVariant::SetGlobalTextureTarget(GLenum target) {
  AutoLock lock(lock_);
  LOG_ALWAYS_FATAL_IF(IsCompileRequested());
  LOG_ALWAYS_FATAL_IF(!IsValidTextureTargetLimited(target));
  global_texture_target_ = target;
}

void ShaderVariant::SetSource(const std::string& str) {
  AutoLock lock(lock_);
  LOG_ALWAYS_FATAL_IF(IsCompileRequested());
  original_source_ = str;
  RewriteSource();
}

std::string ShaderVariant::GetInfoLog() const {
  AutoLock lock(lock_);
  if (!global_name_ || !info_log_cache_.IsDirty()) {
    return info_log_cache_.Get();
  }

  GLint info_log_length = 0;
  GlesContext* ctx = GetRequiredContext();
  PASS_THROUGH(ctx, GetShaderiv, global_name_,
               GL_INFO_LOG_LENGTH, &info_log_length);

  if (info_log_length > 0) {
    char* buf = new char[info_log_length];
    PASS_THROUGH(ctx, GetShaderInfoLog, global_name_,
                 info_log_length, NULL, buf);
    info_log_cache_.Mutate() = buf;
    delete[] buf;
  } else {
    info_log_cache_.Mutate() = "";
  }

  info_log_cache_.Clean();
  return info_log_cache_.Get();
}

bool ShaderVariant::VerifySuccessfulCompile() const {
  AutoLock lock(lock_);
  if (compile_status_ == COMPILE_ATTEMPTED) {
    GLint compileStatus = 0;
    GlesContext* ctx = GetRequiredContext();
    PASS_THROUGH(ctx, GetShaderiv, global_name_,
                 GL_COMPILE_STATUS, &compileStatus);
    compile_status_ = (compileStatus != GL_FALSE ?
        COMPILE_SUCCESSFUL : COMPILE_FAILED);
  }
  return (compile_status_ == COMPILE_SUCCESSFUL);
}

void ShaderVariant::Compile() {
  AutoLock lock(lock_);
  LOG_ALWAYS_FATAL_IF(IsCompileRequested());

  GlesContext* ctx = GetRequiredContext();
  EnsureLiveShaderObject(ctx);

  const char* source_ptr = updated_source_.c_str();
  PASS_THROUGH(ctx, ShaderSource, global_name_, 1, &source_ptr, NULL);

  PASS_THROUGH(ctx, CompileShader, global_name_);
  compile_status_ = COMPILE_ATTEMPTED;
  info_log_cache_.Mutate() = "";
}

void ShaderVariant::RewriteSource() {
  // We will be injecting some extra code into the source so we need to
  // reserve just a little extra space for it.
  const size_t extra_source_size = 256;
  updated_source_.reserve(original_source_.size() + extra_source_size);
  updated_source_.clear();

  std::string modified_source = original_source_;

  // Note: ExtractVersion actually modifies the original source, removing the
  // #version line.  One way to prevent this would be to create a temporary
  // copy of the original source, but modifying the original source directly
  // is faster and should not really cause any issues.
  const int version = ExtractVersion(&modified_source);
  if (version > 0) {
    char version_decl[26];
    snprintf(version_decl, sizeof(version_decl), "#version %d\n", version);
    updated_source_ += version_decl;
  }

  // TODO(crbug.com/441937): Parse the extension correctly by handling things
  // like whitespace as well as enable/warn/disable options.
  if (global_texture_target_ == GL_TEXTURE_2D) {
    Substitute(&modified_source,
               "#extension GL_OES_EGL_image_external : require",
               "                                              ");
    // TODO(crbug.com/441937): Parse the declarations correctly so these
    // substitutions are only applied to the intended uniform declarations.
    has_replaced_external_texture_with_2d_ =
        Substitute(&modified_source,
                   " samplerExternalOES ",
                   " sampler2D          ");
  } else {
    // The underlying texture is external - do not change the program.
    has_replaced_external_texture_with_2d_ = false;
  }

  // TODO(crbug.com/422014): Remove these subtitutions when we either can turn
  // off the compiler error in Chrome, or if we decided to require developers
  // running under ARC to conform to the GLES standard.
  //
  // We strip out these specific default precision statements as a workaround
  // for a handful of apps. PepperGL follows the GLES standard, and requires
  // that the qualifiers used on uniforms shared between the vertex and
  // fragment shaders match. Android does not perform that check, and there
  // are some apps that therefore fail to run.
  //
  // Note that there is still possible to have a mismatch if the shader uses
  // preprocessor definitions to generate the default precision statement,
  // or if the shader uses precision qualifiers when declaring the uniforms
  // (not using the defaults). We should have a better answer before it becomes
  // necessary to do something more complicated than this.
  Substitute(&modified_source,
             "precision highp float;",
             "                      ");
  Substitute(&modified_source,
             "precision mediump float;",
             "                        ");
  Substitute(&modified_source,
             "precision lowp float;",
             "                     ");
  // The consequence of stripping out the default precision statements is that
  // we need to add one back in. Fragment shaders in particular have no default
  // precision defined for floating point values, which is also an error.
  updated_source_ += "precision highp float;\n";
  updated_source_ += "#line 1\n";
  updated_source_ += modified_source;
}

// static
bool ShaderVariant::Substitute(std::string* src, const char* match,
                               const char* repl) {
  const size_t match_len = strlen(match);
  const size_t repl_len = strlen(repl);
  LOG_ALWAYS_FATAL_IF(match_len != repl_len);

  bool any_matches = false;
  size_t idx = src->find(match);
  while (idx != std::string::npos) {
    any_matches = true;
    for (size_t i = 0; i < match_len; ++i) {
      (*src)[idx + i] = repl[i];
    }
    idx = src->find(match);
  }
  return any_matches;
}

// static
int ShaderVariant::ExtractVersion(std::string* src) {
  // Find and remove the #version token if it exists.
  const int min_version = 100;
  enum State {
    PARSE_NONE,
    PARSE_IN_C_COMMENT,
    PARSE_IN_LINE_COMMENT
  };

  int version = min_version;
  State state = PARSE_NONE;

  size_t idx = 0;
  while (idx < src->size()) {
    char curr = src->at(idx);
    char next = src->at(idx + 1);

    if (state == PARSE_IN_C_COMMENT) {
      if (curr == '*' && next == '/') {
        state = PARSE_NONE;
        idx += 2;
      } else {
        ++idx;
      }
    } else if (state == PARSE_IN_LINE_COMMENT) {
      if (curr == '\n') {
        state = PARSE_NONE;
      }
      ++idx;
    } else if (curr == '/' && next == '/') {
      state = PARSE_IN_LINE_COMMENT;
      idx += 2;
    } else if (curr == '/' && next == '*') {
      state = PARSE_IN_C_COMMENT;
      idx += 2;
    } else if (curr == ' ' || curr == '\t' || curr == '\r' || curr == '\n') {
      ++idx;
    } else {
      // We have reached the first non-blank character outside a comment.  If
      // there is a #version token in the source, then it must be here.
      char* c = &src->at(idx);
      // TODO(crbug.com/441937): Make this more robust.  It should be able to
      // correctly handle something like "#version123".
      if (!strncmp(c, "#version", 8)) {
        int ver;
        if (sscanf(c + 8, "%d", &ver) == 1) {  // NOLINT(runtime/printf)
          // Blank out the version token from the source.
          for (int i = 0; i < 8; i++, c++)
            *c = ' ';
          while (*c < '0' || *c > '9') {
            *c = ' ';
            ++c;
          }
          while (*c >= '0' && *c <= '9') {
            *c = ' ';
            ++c;
          }
          version = std::max(version, ver);
        }
      }
      break;
    }
  }
  return version;
}
