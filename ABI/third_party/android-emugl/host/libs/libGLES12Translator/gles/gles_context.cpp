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

#include "gles/gles_context.h"

#include <GLES/gl.h>
#include <GLES/glext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <algorithm>
#include <stdio.h>
#include <vector>

#include "common/alog.h"
#include "common/dlog.h"
#include "gles/gles_options.h"
#include "gles/debug.h"
#include "gles/framebuffer_data.h"
#include "gles/gles_utils.h"
#include "gles/macros.h"
#include "gles/program_data.h"
#include "gles/renderbuffer_data.h"
#include "gles/shader_data.h"
#include "gles/state.h"
#include "gles/texture_data.h"

namespace {

#define VERTEX_ATTRIBUTE_KEY(enum, name) name,
const char* kVertexAttributeNames[] = {VERTEX_ATTRIBUTE_KEY_TUPLE};
#undef VERTEX_ATTRIBUTE_KEY

uint32_t Hash(const char* str) {
  uint32_t res = 0;
  while (*str) {
    res = (res * 101)  +  *str++;
  }
  return res;
}

void DeleteShader(GLuint* shader) {
  if (GetCurrentGlesContext()) {
    PASS_THROUGH(GetCurrentGlesContext(), DeleteShader, *shader);
  }
}

void DeleteProgram(ProgramContext* program) {
  if (GetCurrentGlesContext()) {
    program->Delete();
  }
}

}  // namespace

GlesContext::GlesContext(int32_t id, GlesVersion version, GlesContext* share,
                         void* underlying_context,
                         const UnderlyingApis* underlying_apis)
    : uniform_context_(this),
      texture_context_(this),
      pointer_context_(this),
      array_buffer_binding_(0),
      element_buffer_binding_(0),
      framebuffer_binding_(0),
      renderbuffer_binding_(0),
      version_(version),
      id_(id + version * 100000),
      initialized_(false),
      initialized_viewport_(false),
      error_(GL_NO_ERROR),
      gles_impl_(underlying_context),
      apis_(underlying_apis),
      share_group_(NULL),
      global_override_framebuffer_(0),
      global_framebuffer_(0),
      global_depth_renderbuffer_(0),
      global_stencil_renderbuffer_(0),
      cached_draw_width_(0),
      cached_draw_height_(0),
      cached_draw_stencil_size_(0),
      cached_draw_depth_size_(0),
      cached_global_texture_(0),
      vertex_shader_cache_(kCacheLimit, &DeleteShader),
      fragment_shader_cache_(kCacheLimit, &DeleteShader),
      program_cache_(kCacheLimit, &DeleteProgram),
      fullscreen_quad_(NULL),
      checks_enabled_(emugl::GlesOptions::GLErrorChecksEnabled()),
      global_extensions_(NULL),
      supports_packed_depth_stencil_(false) {
  if (share) {
    share_group_ = share->GetShareGroup();
  } else {
    share_group_ = ShareGroupPtr(new ShareGroup(this));
  }
}

GlesContext::~GlesContext() {
  DeleteFramebufferOverride();
  delete fullscreen_quad_;
}

void GlesContext::OnMakeCurrent() {
    Restore();
}

void GlesContext::Invalidate() {
  if (initialized_) {
    pointer_context_.Release();
    initialized_ = false;
  }
}

void GlesContext::Restore() {
  if (initialized_) {
      DLOG("Context already initialized, go away.");
    return;
  }

  const int max_texture_units = UniformContext::kMaxTextureUnits;
  DLOG("Initialize pointer, texture, and uniform contexts.");
  if (version_ == kGles11) {
    pointer_context_.Init(kNumVertexAttributeKeys);
  } else if (version_ == kGles20) {
    pointer_context_.Init(max_vertex_attribs_.Get());
  } else {
    LOG_ALWAYS_FATAL("Unknown GLES version %d", version_);
  }
  texture_context_.Init(max_texture_units, max_texture_size_.Get());
  uniform_context_.Init(max_texture_units);

  if (version_ == kGles11) {
      GLfloat values[4];
      DLOG("Pass through glVertexAttrib4fv");
      PASS_THROUGH(this, VertexAttrib4fv, kColorVertexAttribute,
              uniform_context_.GetColor().GetFloatArray(values));
      PASS_THROUGH(this, VertexAttrib4fv, kNormalVertexAttribute,
              uniform_context_.GetNormal().GetFloatArray(values));
      PASS_THROUGH(this, VertexAttrib1f, kPointSizeVertexAttribute,
              uniform_context_.GetPointParameters().current_size);
  }

  initialized_ = true;
}

void GlesContext::OnAttachSurface(SurfaceControlCallbackPtr sfc,
                                  GLint width, GLint height) {
  surface_callback_ = sfc;
  if (!initialized_viewport_) {
    initialized_viewport_ = true;

    // The first time that a context is attached to a surface, the context needs
    // to be initialized with the dimensions of the draw surface. These
    // dimensions are used to initialized the viewport and scissor rectangles
    // correctly for the surface.
    // It is the client's responsibility to maintain those values afterwards.
    viewport_.Mutate()[0] = 0;
    viewport_.Mutate()[1] = 0;
    viewport_.Mutate()[2] = width;
    viewport_.Mutate()[3] = height;
    PASS_THROUGH(this, Viewport, 0, 0, width, height);
    PASS_THROUGH(this, Scissor, 0, 0, width, height);
  }
}

void GlesContext::UpdateFramebufferOverride(GLint width, GLint height,
                                            GLint depth_size,
                                            GLint stencil_size,
                                            GLuint global_texture_name) {
  EnsureUnderlyingExtensionsKnown();

  const bool width_changed = cached_draw_width_ != width;
  const bool height_changed = cached_draw_height_ != height;
  const bool depth_changed = cached_draw_depth_size_ != depth_size;
  const bool stencil_changed = cached_draw_stencil_size_ != stencil_size;
  const bool texture_changed = cached_global_texture_ != global_texture_name;

  if (global_texture_name) {
    if (width_changed || height_changed || depth_changed || stencil_changed) {
      DeleteFramebufferOverride();
      PASS_THROUGH(this, GenFramebuffers, 1, &global_framebuffer_);
    }

    if (width_changed || height_changed || texture_changed) {
      SetupColorTextureAttachment(global_texture_name);
    }

    if (width_changed || height_changed || depth_changed || stencil_changed) {
      SetupDepthStencilAttachment(width, height, depth_size, stencil_size);
    }

    if (AreChecksEnabled()) {
      const GLint status = PASS_THROUGH(this, CheckFramebufferStatus,
                                        GL_FRAMEBUFFER);
      LOG_ALWAYS_FATAL_IF(status != GL_FRAMEBUFFER_COMPLETE,
                          "Error overriding framebuffer: 0x%x", status);
    }
    global_override_framebuffer_ = global_framebuffer_;
  } else {
    global_override_framebuffer_ = 0;
  }

  cached_draw_width_ = width;
  cached_draw_height_ = height;
  cached_draw_depth_size_ = depth_size;
  cached_draw_stencil_size_ = stencil_size;
  cached_global_texture_ = global_texture_name;

  // Restore original bindings as they may have been overridden when updating
  // the framebuffer attachments.
  PASS_THROUGH(this, BindRenderbuffer, GL_RENDERBUFFER,
               share_group_->GetRenderbufferGlobalName(renderbuffer_binding_));
  if (framebuffer_binding_ != 0) {
    PASS_THROUGH(this, BindFramebuffer, GL_FRAMEBUFFER,
                 share_group_->GetFramebufferGlobalName(framebuffer_binding_));
  } else {
    PASS_THROUGH(this, BindFramebuffer, GL_FRAMEBUFFER,
                 global_override_framebuffer_);
  }
}

void GlesContext::SetupColorTextureAttachment(GLuint texture) {
  PASS_THROUGH(this, BindFramebuffer, GL_FRAMEBUFFER, global_framebuffer_);
  PASS_THROUGH(this, FramebufferTexture2D, GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
               GL_TEXTURE_2D, texture, 0);
}

void GlesContext::SetupDepthStencilAttachment(GLint width, GLint height,
                                              GLint depth_size,
                                              GLint stencil_size) {
  if (supports_packed_depth_stencil_ && depth_size && stencil_size) {
    LOG_ALWAYS_FATAL_IF(depth_size > 24 || depth_size < 0);
    LOG_ALWAYS_FATAL_IF(stencil_size > 8 || stencil_size < 0);

    // Both the depth and stencil buffer actually refer to the same object.
    PASS_THROUGH(this, GenRenderbuffers, 1, &global_depth_renderbuffer_);
    global_stencil_renderbuffer_ = global_depth_renderbuffer_;

    PASS_THROUGH(this, BindRenderbuffer, GL_RENDERBUFFER,
                 global_depth_renderbuffer_);
    PASS_THROUGH(this, RenderbufferStorage, GL_RENDERBUFFER,
                 GL_DEPTH24_STENCIL8_OES, width, height);
  } else {
    if (depth_size) {
      LOG_ALWAYS_FATAL_IF(depth_size > 16 || depth_size < 0);

      PASS_THROUGH(this, GenRenderbuffers, 1, &global_depth_renderbuffer_);
      PASS_THROUGH(this, BindRenderbuffer, GL_RENDERBUFFER,
                   global_depth_renderbuffer_);
      PASS_THROUGH(this, RenderbufferStorage, GL_RENDERBUFFER,
                   GL_DEPTH_COMPONENT16, width, height);
    }
    if (stencil_size) {
      LOG_ALWAYS_FATAL_IF(stencil_size > 8 || stencil_size < 0);

      PASS_THROUGH(this, GenRenderbuffers, 1,
                   &global_stencil_renderbuffer_);
      PASS_THROUGH(this, BindRenderbuffer, GL_RENDERBUFFER,
                   global_stencil_renderbuffer_);
      PASS_THROUGH(this, RenderbufferStorage, GL_RENDERBUFFER,
                   GL_STENCIL_INDEX8, width, height);
    }
  }

  PASS_THROUGH(this, BindFramebuffer, GL_FRAMEBUFFER, global_framebuffer_);
  PASS_THROUGH(this, FramebufferRenderbuffer, GL_FRAMEBUFFER,
               GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
               global_depth_renderbuffer_);
  PASS_THROUGH(this, FramebufferRenderbuffer, GL_FRAMEBUFFER,
               GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
               global_stencil_renderbuffer_);
}

void GlesContext::DeleteFramebufferOverride() {
  // Note that it is safe to delete the same renderbuffer object multiple
  // times. The GLES specification requires the duplicate delete to be
  // silently ignored.
  const GLuint renderbuffers[2] = { global_depth_renderbuffer_,
                                    global_stencil_renderbuffer_ };
  PASS_THROUGH(this, DeleteFramebuffers, 1, &global_framebuffer_);
  PASS_THROUGH(this, DeleteRenderbuffers, 2, renderbuffers);
  global_framebuffer_ = 0;
  global_depth_renderbuffer_ = 0;
  global_stencil_renderbuffer_ = 0;
}

ShareGroupPtr GlesContext::GetShareGroup() const {
  LOG_ALWAYS_FATAL_IF(share_group_ == NULL);
  return share_group_;
}

void GlesContext::SetGLerror(GLenum error, const char* function,
                             int line_number, const char* fmt, ...) {
  const bool set_error = (error_ == GL_NO_ERROR);
  if (set_error) {
    error_ = error;
  }

  char msg[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, sizeof(msg), fmt, args);
  va_end(args);

  char error_str[512];
  snprintf(error_str, sizeof(error_str), "%s:%d [ctx=%d]: %s", function,
           line_number, id_, msg);

  bool display_error = false;
  bool suppress_output = false;
  if (AreChecksEnabled()) {
    display_error = true;
  } else {
    const size_t num = error_counter_.size();
    if (num == kMaximumErrorCounterSize) {
      ALOGE("IMPORTANT: [ctx=%d] Too many errors!  Suppressing GLError output!",
            id_);
      // Add some dummy data until the error_counter_ exceeds the limit to
      // prevent this message from being printed again.
      uint32_t i = 0;
      while (error_counter_.size() == kMaximumErrorCounterSize) {
        error_counter_[i++] = 0;
      }
    } else if (num < kMaximumErrorCounterSize) {
      const uint32_t hash = Hash(error_str);
      uint32_t& count = error_counter_[hash];
      display_error = (count <= kErrorLimit);
      suppress_output = (count == kErrorLimit);
      ++count;
    }
  }

  if (display_error) {
    if (set_error) {
      ALOGW("[ctx=%d] GLES Error Set: %s (0x%x)", id_, GetEnumString(error),
            error);
    }
    ALOGE("%s", error_str);
    if (suppress_output) {
      ALOGE("IMPORTANT: Error \"%s\" will no longer be logged to stderr!",
            error_str);
    }
  }
}

GLenum GlesContext::GetGLerror() {
  const GLenum err = error_;
  error_ = GL_NO_ERROR;
  return err;
}

bool GlesContext::AreChecksEnabled() const {
  return checks_enabled_;
}

void GlesContext::Flush() {
  PASS_THROUGH(this, Flush);
}

void GlesContext::EnsureSurfaceReadyToDraw() const {
  if (surface_callback_ != NULL) {
    surface_callback_->EnsureBufferReady();
  }
}

void GlesContext::SetCurrentUserProgram(const ProgramDataPtr& program) {
  if (program == current_user_program_) {
    return;
  }

  if (program != NULL) {
    if (!program->Use(true)) {
      return;
    }
  }

  if (current_user_program_ != NULL) {
    current_user_program_->Use(false);
  }

  current_user_program_ = program;
}

bool GlesContext::CanDraw() const {
  if (version_ == kGles11) {
    if (!pointer_context_.IsArrayEnabled(kPositionVertexAttribute)) {
      return false;
    }
    if (IsEnabled(GL_MATRIX_PALETTE_OES)) {
      if (!pointer_context_.IsArrayEnabled(kWeightVertexAttribute)) {
        return false;
      }
      if (!pointer_context_.IsArrayEnabled(kMatrixIndexVertexAttribute)) {
        return false;
      }
      const PointerData* index_ptr = pointer_context_.GetPointerData(
          kMatrixIndexVertexAttribute);
      if (index_ptr->size == 0) {
        return false;
      }
    }
    return true;
  } else {
    return current_user_program_ != NULL;
  }
}

void GlesContext::DrawFullscreenQuad(GLuint texture, bool flip_v) {
  if (fullscreen_quad_ == NULL) {
    fullscreen_quad_ = new FullscreenQuad(this);
  }

  EnsureSurfaceReadyToDraw();

  fullscreen_quad_->Draw(texture, flip_v);
}

void GlesContext::Draw(DrawType draw, GLenum mode, GLint first, GLsizei count,
            GLenum type, const GLvoid* indices) {
  LOG_ALWAYS_FATAL_IF(draw != kDrawArrays && draw != kDrawElements);
  if (!CanDraw()) {
    return;
  }

  EnsureSurfaceReadyToDraw();

  bool program_uses_external_as_2d = false;
  PrepareProgramObject(mode, &program_uses_external_as_2d);
  texture_context_.PrepareTextures(version_ == kGles11,
                                   program_uses_external_as_2d);

  if (draw == kDrawArrays) {
    pointer_context_.PrepareBuffersForDrawArrays(first, count);
    PASS_THROUGH(this, DrawArrays, mode, first, count);
  } else {
    indices = pointer_context_.PrepareBuffersForDrawElements(count, type,
                                                             indices);
    PASS_THROUGH(this, DrawElements, mode, count, type, indices);
  }

  // Restore the buffer bindings as the pointer context may have changed them.
  PASS_THROUGH(this, BindBuffer, GL_ARRAY_BUFFER,
               share_group_->GetBufferGlobalName(array_buffer_binding_));
  PASS_THROUGH(this, BindBuffer, GL_ELEMENT_ARRAY_BUFFER,
               share_group_->GetBufferGlobalName(element_buffer_binding_));
  texture_context_.RestoreTextures();
  ClearProgramObject();
}

bool GlesContext::BindImageToTexture(GLenum target, EglImagePtr image) {
  TextureDataPtr texture = GetBoundTextureData(target);
  if (texture == NULL) {
    return false;
  }

  // Delete old texture object but only if it is not a target of a EGLImage
  const GLuint name =
      share_group_->GetTextureGlobalName(texture->GetLocalName());
  if (name) {
    if (!texture->IsEglImageAttached()) {
      PASS_THROUGH(this, DeleteTextures, 1, &name);
    }
  }

  // Map the texture to the EGL image texture.
  texture->AttachEglImage(image);
  share_group_->SetTextureGlobalName(texture->GetLocalName(),
                                     image->global_texture_name);

  // This function is always called to create a TEXTURE_EXTERNAL_OES,
  // Since we are binding the texture, ensure that we know what actual
  // global target this texture uses in the underlying implementation.
  texture_context_.SetTargetTexture(GL_TEXTURE_EXTERNAL_OES,
                                    texture->GetLocalName(),
                                    image->global_texture_target);

  // Bind the EGL image texture (which is now the same as the texture).
  PASS_THROUGH(this, BindTexture, image->global_texture_target,
               image->global_texture_name);
  return true;
}

bool GlesContext::BindImageToRenderbuffer(EglImagePtr image) {
  RenderbufferDataPtr rb = GetBoundRenderbufferData();
  if (rb == NULL) {
    return false;
  }

  rb->SetEglImage(image);

  // If the renderbuffer is attached to a framebuffer, change the
  // framebuffer attachment to point to the EGLImage texture object.
  if (rb->IsAttached()) {
    const bool save_and_restore =
        (framebuffer_binding_ != rb->GetAttachedFramebuffer());
    if (save_and_restore) {
      const GLuint global_name =
          share_group_->GetFramebufferGlobalName(rb->GetAttachedFramebuffer());
      PASS_THROUGH(this, BindFramebuffer, GL_FRAMEBUFFER, global_name);
    }
    PASS_THROUGH(this, FramebufferTexture2D, GL_FRAMEBUFFER,
                 rb->GetAttachment(), image->global_texture_target,
                 image->global_texture_name, 0);
    if (save_and_restore) {
      const GLuint global_name =
          share_group_->GetFramebufferGlobalName(framebuffer_binding_);
      PASS_THROUGH(this, BindFramebuffer, GL_FRAMEBUFFER, global_name);
    }
  }
  return true;
}


void GlesContext::BindFramebuffer(GLuint framebuffer) {
  framebuffer_binding_ = framebuffer;
  GLuint global_name = 0;

  if (framebuffer == 0) {
    // Use whatever framebuffer is set as the override framebuffer in place of
    // the default framebuffer.
    global_name = global_override_framebuffer_;
  } else {
    global_name = share_group_->GetFramebufferGlobalName(framebuffer);
  }
  PASS_THROUGH(this, BindFramebuffer, GL_FRAMEBUFFER, global_name);
}

void GlesContext::DrawTex(GLfloat x, GLfloat y, GLfloat z, GLfloat width,
                          GLfloat height) {
  // Backup vbo's
  const GLint array_buffer = array_buffer_binding_;
  const GLint element_buffer = element_buffer_binding_;
  PASS_THROUGH(this, BindBuffer, GL_ARRAY_BUFFER, 0);
  PASS_THROUGH(this, BindBuffer, GL_ELEMENT_ARRAY_BUFFER, 0);

  // Backup pointers
  const PointerContext::PointerDataVector pointers = pointer_context_.GetPointers();
  for (GLint index = 0; index < kNumVertexAttributeKeys; ++index) {
    pointer_context_.DisableArray(index);
    PASS_THROUGH(this, DisableVertexAttribArray, index);
  }

  // Setup projection matrix to draw in viewport aligned coordinates
  const emugl::Matrix projection_matrix = uniform_context_.GetProjectionMatrix();
  const GLint* viewport = viewport_.Get();
  uniform_context_.MutateProjectionMatrix() = emugl::Matrix::GenerateOrthographic(
      viewport[0], viewport[0] + viewport[2], viewport[1],
      viewport[1] + viewport[3], 0.f, -1.f);

  // Setup modelview matrix
  const emugl::Matrix model_view_matrix = uniform_context_.GetModelViewMatrix();
  uniform_context_.MutateModelViewMatrix().AssignIdentity();

  // Setup texture matrix
  emugl::Matrix texture_matrices[UniformContext::kMaxTextureUnits];
  for (GLint stage = 0; stage < UniformContext::kMaxTextureUnits; ++stage) {
    texture_matrices[stage] = uniform_context_.GetTextureMatrixByStage(stage);
    uniform_context_.MutateTextureMatrixByStage(stage).AssignIdentity();
  }

  // Backup |enabled_set_| and disable clip planes and lighting.
  const std::set<GLenum> enabled_set = enabled_set_;
  for (GLint i = 0; i < UniformContext::kMaxClipPlanes; ++i) {
    enabled_set_.erase(GL_CLIP_PLANE0 + i);
  }
  enabled_set_.erase(GL_LIGHTING);

  GLfloat texels[UniformContext::kMaxTextureUnits][8] = {};
  GLint num_textures = 0;
  for (GLint i = 0; i < UniformContext::kMaxTextureUnits; ++i) {
    if (!texture_context_.IsEnabled(GL_TEXTURE0 + i, GL_TEXTURE_2D)) {
      continue;
    }
    const GLint texture = texture_context_.GetTexture(GL_TEXTURE0 + i,
                                                      GL_TEXTURE_2D);
    const TextureDataPtr texture_data = share_group_->GetTextureData(texture);
    if (texture_data == NULL) {
      continue;
    }

    const GLint* crop_rect = texture_data->GetCropRect();
    const GLfloat tex_width = texture_data->GetWidth();
    const GLfloat tex_height = texture_data->GetHeight();

    texels[i][0] = crop_rect[0] / tex_width;
    texels[i][1] = crop_rect[1] / tex_height;

    texels[i][2] = crop_rect[0] / tex_width;
    texels[i][3] = (crop_rect[1] + crop_rect[3]) / tex_height;

    texels[i][4] = (crop_rect[0] + crop_rect[2]) / tex_width;
    texels[i][5] = (crop_rect[1] + crop_rect[3]) / tex_height;

    texels[i][6] = (crop_rect[0] + crop_rect[2]) / tex_width;
    texels[i][7] = crop_rect[1] / tex_height;


    pointer_context_.EnableArray(kTexCoord0VertexAttribute + i);
    PASS_THROUGH(this, EnableVertexAttribArray, kTexCoord0VertexAttribute + i);
    PASS_THROUGH(this, VertexAttribPointer, kTexCoord0VertexAttribute + i, 2,
                 GL_FLOAT, GL_FALSE, 0, texels[i]);
    num_textures++;
  }

  if (num_textures > 0) {
    z = ClampValue(z, 0.0f, 1.0f);
    const GLfloat vertices[] = {
      x, y, z,
      x, y + height, z,
      x + width, y + height, z,
      x + width, y, z,
    };
    pointer_context_.EnableArray(kPositionVertexAttribute);
    PASS_THROUGH(this, EnableVertexAttribArray, kPositionVertexAttribute);
    PASS_THROUGH(this, VertexAttribPointer, kPositionVertexAttribute, 3,
                 GL_FLOAT, GL_FALSE, 0, vertices);

    // DrawTex() needs texture environments etc, so we use GLES1 emulation to
    // avoid unnecessary complexity.
    Draw(kDrawArrays, GL_TRIANGLE_FAN, 0, 4, 0, 0);
  }

  // Restore enabled_set_
  enabled_set_ = enabled_set;

  // Restore matrix state
  uniform_context_.MutateProjectionMatrix() = projection_matrix;
  uniform_context_.MutateModelViewMatrix() = model_view_matrix;
  for (GLint stage = 0; stage < UniformContext::kMaxTextureUnits; ++stage) {
    uniform_context_.MutateTextureMatrixByStage(stage) =
        texture_matrices[stage];
  }

  // Restore pointers
  for (GLuint index = 0; index < pointers.size(); ++index) {
    const PointerData& ptr = pointers[index];
    PASS_THROUGH(this, BindBuffer, GL_ARRAY_BUFFER,
                 share_group_->GetBufferGlobalName(ptr.buffer_name));
    PASS_THROUGH(this, VertexAttribPointer, index, ptr.size, ptr.type,
                 ptr.normalize, ptr.stride, ptr.pointer);
    if (ptr.enabled) {
      PASS_THROUGH(this, EnableVertexAttribArray, index);
    } else {
      PASS_THROUGH(this, DisableVertexAttribArray, index);
    }
  }
  pointer_context_.SetPointers(pointers);

  // Restore vbo's
  PASS_THROUGH(this, BindBuffer, GL_ARRAY_BUFFER,
               share_group_->GetBufferGlobalName(array_buffer));
  PASS_THROUGH(this, BindBuffer, GL_ELEMENT_ARRAY_BUFFER,
               share_group_->GetBufferGlobalName(element_buffer));
}

BufferDataPtr GlesContext::GetBoundTargetBufferData(GLenum target) {
  if (target == GL_ARRAY_BUFFER) {
    return share_group_->GetBufferData(array_buffer_binding_);
  } else if (target == GL_ELEMENT_ARRAY_BUFFER) {
    return share_group_->GetBufferData(element_buffer_binding_);
  } else {
    return BufferDataPtr();
  }
}

FramebufferDataPtr GlesContext::GetBoundFramebufferData() {
  return share_group_->GetFramebufferData(framebuffer_binding_);
}

RenderbufferDataPtr GlesContext::GetBoundRenderbufferData() {
  return share_group_->GetRenderbufferData(renderbuffer_binding_);
}

TextureDataPtr GlesContext::GetBoundTextureData(GLenum target) {
  const GLuint name = texture_context_.GetBoundTexture(target);
  if (name == 0) {
    return texture_context_.GetDefaultTextureData(target);
  }
  return share_group_->GetTextureData(name);
}

GLenum GlesContext::GetEnabledTextureTarget(GLenum id) const {
  if (texture_context_.IsEnabled(id, GL_TEXTURE_CUBE_MAP)) {
    return GL_TEXTURE_CUBE_MAP;
  } else if (texture_context_.IsEnabled(id, GL_TEXTURE_EXTERNAL_OES)) {
    return GL_TEXTURE_EXTERNAL_OES;
  } else if (texture_context_.IsEnabled(id, GL_TEXTURE_2D)) {
    return GL_TEXTURE_2D;
  } else {
    return GL_NONE;
  }
}

ShaderConfig GlesContext::ConfigureShader(GLenum mode) {
  ShaderConfig cfg;

  for (int i = 0; i < UniformContext::kMaxTextureUnits; ++i) {
    const GLenum id = GL_TEXTURE0 + i;
    const GLenum target = GetEnabledTextureTarget(id);
    if (target == GL_NONE) {
      continue;
    }

    // Only generate texture shader if texture coord array is enabled, or
    // GL_TEXTURE_CUBE_MAP texture TexGen is enabled.
    // es_full_spec_1.1.12.pdf section 2.7
    // If the GL_TEXTURE_COORD_ARRAY is disabled, the current values specified
    // by glMultiTexCoord4{xf}() should be used. But most GLES1 implementations
    // will consider the texture unit is disabled. To be compatible to other
    // implementations, we will test the GL_TEXTURE_COORD_ARRAY.
    const GLuint index = TextureContext::GetTextureCoordAttrib(id);
    const bool texture_enabled = (target == GL_TEXTURE_CUBE_MAP) ?
                                 uniform_context_.GetTexGen(id)->enabled :
                                 pointer_context_.IsArrayEnabled(index);
    if (!texture_enabled) {
      continue;
    }

    cfg.any_texture_units_enabled = true;

    const TexEnv* env = uniform_context_.GetTexEnv(id);
    const TexGen* texgen = uniform_context_.GetTexGen(id);

    const GLuint texture = texture_context_.GetTexture(id, target);
    TextureDataPtr obj = share_group_->GetTextureData(texture);
    if (obj == NULL && texture == 0) {
      obj = texture_context_.GetDefaultTextureData(target);
    }

    ShaderConfig::TextureConfig& t = cfg.texture[i];
    t.enabled = true;
    t.mode = env->mode;
    t.target = target;
    if (t.mode != GL_COMBINE) {
      t.format = obj->GetFormat();
    }
    t.combine_rgb = env->combine_rgb;
    t.combine_alpha = env->combine_alpha;
    for (int j = 0; j < 3; ++j) {
      t.src_rgb[j] = env->src_rgb[j];
      t.src_alpha[j] = env->src_alpha[j];
      t.operand_rgb[j] = env->operand_rgb[j];
      t.operand_alpha[j] = env->operand_alpha[j];
    }

    if (target == GL_TEXTURE_CUBE_MAP) {
      t.texgen_mode = texgen->mode;
      t.texgen_enabled = texgen->enabled;
      cfg.any_texture_gen_normal_map |= texgen->mode == GL_NORMAL_MAP_OES;
      cfg.any_texture_gen_reflection_map |=
          texgen->mode == GL_REFLECTION_MAP_OES;
    }
  }

  for (int i = 0; i < UniformContext::kMaxLights; ++i) {
    const GLenum id = GL_LIGHT0 + i;
    if (IsEnabled(id)) {
      const Light* light = uniform_context_.GetLight(id);
      ShaderConfig::LightConfig& l = cfg.light[i];
      l.enabled = true;
      l.directional = light->IsDirectional();
      l.spot = light->IsSpot();
      l.attenuate = light->ShouldAttenuate();
    }
  }

  for (int i = 0; i < UniformContext::kMaxClipPlanes; ++i) {
    const GLenum id = GL_CLIP_PLANE0 + i;
    cfg.enable_clip_plane[i] = IsEnabled(id);
    cfg.any_clip_planes_enabled |= IsEnabled(id);
  }

  cfg.mode = mode;
  cfg.enable_alpha_test = IsEnabled(GL_ALPHA_TEST);
  if (cfg.enable_alpha_test) {
    cfg.alpha_func = uniform_context_.GetAlphaTest().func;
  }
  cfg.enable_color_material = IsEnabled(GL_COLOR_MATERIAL);
  cfg.enable_fog = IsEnabled(GL_FOG);
  if (cfg.enable_fog) {
    cfg.fog_mode = uniform_context_.GetFog().mode;
  }
  cfg.enable_light_model_two_side = IsEnabled(GL_LIGHT_MODEL_TWO_SIDE);
  cfg.enable_lighting = IsEnabled(GL_LIGHTING);
  cfg.enable_normalize = IsEnabled(GL_NORMALIZE);
  cfg.enable_point_smooth = IsEnabled(GL_POINT_SMOOTH);
  cfg.enable_point_sprite = IsEnabled(GL_POINT_SPRITE_OES);
  cfg.enable_matrix_palette = IsEnabled(GL_MATRIX_PALETTE_OES);
  if (cfg.enable_matrix_palette) {
    const PointerData* ptr = pointer_context_.GetPointerData(
        kMatrixIndexVertexAttribute);
    cfg.vertex_units = ptr->size;
  }
  return cfg;
}

GLuint GlesContext::CompileShader(GLenum shader_kind, const char* source) {
  GLuint object = PASS_THROUGH(this, CreateShader, shader_kind);
  PASS_THROUGH(this, ShaderSource, object, 1, &source, NULL);
  PASS_THROUGH(this, CompileShader, object);

  if (AreChecksEnabled()) {
    GLint compiled = 0;
    PASS_THROUGH(this, GetShaderiv, object, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
      GLint len = 0, written = 0;
      PASS_THROUGH(this, GetShaderiv, object, GL_INFO_LOG_LENGTH, &len);
      char* log = new char[len];
      PASS_THROUGH(this, GetShaderInfoLog, object, len, &written, log);
      LOG_ALWAYS_FATAL("Unable to compile %s:\n%s\n%s",
                       GetEnumString(shader_kind), source, log);
      delete[] log;
    }
  }
  return object;
}

GLuint GlesContext::CompileProgram(GLuint vertex_shader,
                                   GLuint fragment_shader) {
  GLuint program = PASS_THROUGH(this, CreateProgram);
  PASS_THROUGH(this, AttachShader, program, vertex_shader);
  PASS_THROUGH(this, AttachShader, program, fragment_shader);
  for (int i = 0; i < kNumVertexAttributeKeys; ++i) {
    PASS_THROUGH(this, BindAttribLocation, program, i,
                 kVertexAttributeNames[i]);
  }
  PASS_THROUGH(this, LinkProgram, program);

  if (AreChecksEnabled()) {
    GLint link_status = 0;
    PASS_THROUGH(this, GetProgramiv, program, GL_LINK_STATUS, &link_status);
    if (link_status == GL_FALSE) {
      GLint len = 0, written = 0;
      PASS_THROUGH(this, GetProgramiv, program, GL_INFO_LOG_LENGTH, &len);
      char* log = new char[len];
      PASS_THROUGH(this, GetProgramInfoLog, program, len, &written, log);
      LOG_ALWAYS_FATAL("Unable to link:\nfragment:%d vertex:%d\n%s",
                       fragment_shader, vertex_shader, log);
      delete[] log;
    }
  }
  return program;
}

ProgramContext& GlesContext::BindProgramContext(GLenum mode) {
  // TODO(crbug.com/441922): Figure out actual maximum sizes for these two
  // buffers to minimize the fixed memory footprint we need.
  static const size_t kMaxShaderBufferSize = 65536;
  char buffer[kMaxShaderBufferSize];

  const ShaderConfig cfg = ConfigureShader(mode);

  GLuint* vs = vertex_shader_cache_.Get(cfg);
  if (!vs) {
    GenerateVertexShader(cfg, buffer, sizeof(buffer));
    const GLuint shader = CompileShader(GL_VERTEX_SHADER, buffer);
    vs = vertex_shader_cache_.Push(cfg, shader);
  }

  GLuint* fs = fragment_shader_cache_.Get(cfg);
  if (!fs) {
    GenerateFragmentShader(cfg, buffer, sizeof(buffer));
    const GLuint shader = CompileShader(GL_FRAGMENT_SHADER, buffer);
    fs = fragment_shader_cache_.Push(cfg, shader);
  }

  ProgramContext* program = program_cache_.Get(cfg);
  if (!program) {
    GLuint id = CompileProgram(*vs, *fs);
    program = program_cache_.Push(cfg, ProgramContext(this, id));
  }

  LOG_ALWAYS_FATAL_IF(program == NULL, "Program not created?");
  program->Bind();
  return *program;
}

void GlesContext::PrepareProgramObject(GLenum mode,
                                       bool* program_uses_external_as_2d) {
  if (current_user_program_ != NULL) {
    current_user_program_->PrepareForRendering(program_uses_external_as_2d);
    return;
  }

  ProgramContext& program = BindProgramContext(mode);
  uniform_context_.Bind(&program);

  if (pointer_context_.IsArrayEnabled(kPositionVertexAttribute)) {
    PASS_THROUGH(this, EnableVertexAttribArray, kPositionVertexAttribute);
  }
  if (pointer_context_.IsArrayEnabled(kNormalVertexAttribute)) {
    PASS_THROUGH(this, EnableVertexAttribArray, kNormalVertexAttribute);
  } else {
    GLfloat values[4];
    PASS_THROUGH(this, VertexAttrib3fv, kNormalVertexAttribute,
                 uniform_context_.GetNormal().GetFloatArray(values));
  }
  if (pointer_context_.IsArrayEnabled(kColorVertexAttribute)) {
    PASS_THROUGH(this, EnableVertexAttribArray, kColorVertexAttribute);
  } else {
    GLfloat values[4];
    PASS_THROUGH(this, VertexAttrib4fv, kColorVertexAttribute,
                 uniform_context_.GetColor().GetFloatArray(values));
  }
  for (int i = 0; i < UniformContext::kMaxTextureUnits; ++i) {
    if (GetEnabledTextureTarget(GL_TEXTURE0 + i) != GL_NONE) {
      const int attribute = kTexCoord0VertexAttribute + i;
      if (pointer_context_.IsArrayEnabled(attribute)) {
        PASS_THROUGH(this, EnableVertexAttribArray, attribute);
      }
    }
  }
  if (mode == GL_POINTS) {
    if (pointer_context_.IsArrayEnabled(kPointSizeVertexAttribute)) {
      PASS_THROUGH(this, EnableVertexAttribArray, kPointSizeVertexAttribute);
    } else {
      const PointParameters& params =
          uniform_context_.GetPointParameters();
      PASS_THROUGH(this, VertexAttrib1f, kPointSizeVertexAttribute,
                   params.current_size);
    }
  }
  if (pointer_context_.IsArrayEnabled(kWeightVertexAttribute)) {
    PASS_THROUGH(this, EnableVertexAttribArray, kWeightVertexAttribute);
  }
  if (pointer_context_.IsArrayEnabled(kMatrixIndexVertexAttribute)) {
    PASS_THROUGH(this, EnableVertexAttribArray, kMatrixIndexVertexAttribute);
  }
}

void GlesContext::ClearProgramObject() {
  if (current_user_program_ != NULL) {
    current_user_program_->CleanupAfterRendering();
    return;
  }

  if (pointer_context_.IsArrayEnabled(kPositionVertexAttribute)) {
    PASS_THROUGH(this, DisableVertexAttribArray, kPositionVertexAttribute);
  }
  if (pointer_context_.IsArrayEnabled(kNormalVertexAttribute)) {
    PASS_THROUGH(this, DisableVertexAttribArray, kNormalVertexAttribute);
  }
  if (pointer_context_.IsArrayEnabled(kColorVertexAttribute)) {
    PASS_THROUGH(this, DisableVertexAttribArray, kColorVertexAttribute);
  }
  for (int i = 0; i < UniformContext::kMaxTextureUnits; ++i) {
    if (GetEnabledTextureTarget(GL_TEXTURE0 + i) != GL_NONE) {
      const int attribute = kTexCoord0VertexAttribute + i;
      if (pointer_context_.IsArrayEnabled(attribute)) {
        PASS_THROUGH(this, DisableVertexAttribArray, attribute);
      }
    }
  }
  if (pointer_context_.IsArrayEnabled(kPointSizeVertexAttribute)) {
    PASS_THROUGH(this, DisableVertexAttribArray, kPointSizeVertexAttribute);
  }
  if (pointer_context_.IsArrayEnabled(kWeightVertexAttribute)) {
    PASS_THROUGH(this, DisableVertexAttribArray, kWeightVertexAttribute);
  }
  if (pointer_context_.IsArrayEnabled(kMatrixIndexVertexAttribute)) {
    PASS_THROUGH(this, DisableVertexAttribArray, kMatrixIndexVertexAttribute);
  }

  PASS_THROUGH(this, UseProgram, 0);
}

// Note: This should be kept in sync with EmulatedCompressedTextureFormats
// in gles_validate.cpp.
static const GLenum kEmulatedCompressedTextureFormats[] = {
    GL_PALETTE4_R5_G6_B5_OES,
    GL_PALETTE4_RGB5_A1_OES,
    GL_PALETTE4_RGB8_OES,
    GL_PALETTE4_RGBA4_OES,
    GL_PALETTE4_RGBA8_OES,
    GL_PALETTE8_R5_G6_B5_OES,
    GL_PALETTE8_RGB5_A1_OES,
    GL_PALETTE8_RGB8_OES,
    GL_PALETTE8_RGBA4_OES,
    GL_PALETTE8_RGBA8_OES,
    GL_ETC1_RGB8_OES,
};

void GlesContext::EnsureCompressedTextureFormatStateKnown() const {
  // TODO(crbug.com/426083): Load the list of compressed formats when needed
  // once for all contexts, since it should be consistent for all of them.
  // Once we know what compressed texture formats are supported, we do not have
  // to compute it again.
  if (!emulated_compressed_texture_formats_.empty()) {
    return;
  }

  // Get the list of formats supported by the underlying implementation.
  GLint underlying_num_compressed_textures = 0;
  PASS_THROUGH(this, GetIntegerv, GL_NUM_COMPRESSED_TEXTURE_FORMATS,
               &underlying_num_compressed_textures);

  std::vector<GLint> underlying_compressed_textures_supported(
      underlying_num_compressed_textures);

  if (underlying_num_compressed_textures) {
    PASS_THROUGH(this, GetIntegerv, GL_COMPRESSED_TEXTURE_FORMATS,
                 &underlying_compressed_textures_supported[0]);
  }

  // Android/GLES1 requires support for a specific list of formats. If they are
  // not supported in the underlying implementation we will have to emulate
  // support by converting texture data.
  const size_t count_emulated_formats =
      sizeof(kEmulatedCompressedTextureFormats) /
      sizeof(kEmulatedCompressedTextureFormats[0]);
  emulated_compressed_texture_formats_.reserve(count_emulated_formats);
  for (size_t i = 0; i < count_emulated_formats; ++i) {
    GLenum format = kEmulatedCompressedTextureFormats[i];
    std::vector<GLint>::const_iterator iter =
        std::find(underlying_compressed_textures_supported.begin(),
                  underlying_compressed_textures_supported.end(), format);
    if (iter == underlying_compressed_textures_supported.end()) {
      emulated_compressed_texture_formats_.push_back(format);
    }
  }

  // Build the list of texture formats we support by combining the list from the
  // underlying implementation with the list of formats we will emulate.
  supported_compressed_texture_formats_.reserve(
      emulated_compressed_texture_formats_.size() +
      underlying_compressed_textures_supported.size());
  copy(emulated_compressed_texture_formats_.begin(),
       emulated_compressed_texture_formats_.end(),
       back_inserter(supported_compressed_texture_formats_));
  copy(underlying_compressed_textures_supported.begin(),
       underlying_compressed_textures_supported.end(),
       back_inserter(supported_compressed_texture_formats_));
}

void GlesContext::EnsureUnderlyingExtensionsKnown() const {
  // TODO(crbug.com/426083): Load the list of extensions when needed once for
  // all contexts, since it should be consistent for all of them.
  if (global_extensions_) {
    return;
  }
  global_extensions_ = reinterpret_cast<const char *>(
      PASS_THROUGH(this, GetString, GL_EXTENSIONS));
  supports_packed_depth_stencil_ =
      strstr(global_extensions_, "OES_packed_depth_stencil") != NULL;
}


bool GlesContext::IsCompressedFormatEmulationNeeded(GLenum format) const {
  EnsureCompressedTextureFormatStateKnown();

  TextureFormats::const_iterator iter =
      std::find(emulated_compressed_texture_formats_.begin(),
                emulated_compressed_texture_formats_.end(), format);

  return iter != emulated_compressed_texture_formats_.end();
}

template <typename T>
bool GlesContext::GetValue(GLenum value, T* data) const {
  switch (value) {
    // Current framebuffer object state.
    // Note we cache the default framebuffer state, but do not currently cache
    // the state of any bound framebuffer.
    case GL_ALPHA_BITS:
      if (framebuffer_binding_ == 0) {
        Convert(data, default_framebuffer_alpha_bits_.Get());
        return true;
      }
      return false;
    case GL_BLUE_BITS:
      if (framebuffer_binding_ == 0) {
        Convert(data, default_framebuffer_blue_bits_.Get());
        return true;
      }
      return false;
    case GL_DEPTH_BITS:
      if (framebuffer_binding_ == 0) {
        Convert(data, default_framebuffer_depth_bits_.Get());
        return true;
      }
      return false;
    case GL_GREEN_BITS:
      if (framebuffer_binding_ == 0) {
        Convert(data, default_framebuffer_green_bits_.Get());
        return true;
      }
      return false;
    case GL_RED_BITS:
      if (framebuffer_binding_ == 0) {
        Convert(data, default_framebuffer_red_bits_.Get());
        return true;
      }
      return false;
    case GL_STENCIL_BITS:
      if (framebuffer_binding_ == 0) {
        Convert(data, default_framebuffer_stencil_bits_.Get());
        return true;
      }
      return false;
    case GL_SAMPLE_BUFFERS:
      if (framebuffer_binding_ == 0) {
        Convert(data, default_framebuffer_sample_buffers_.Get());
        return true;
      }
      return false;

    // This state matches what can be obtained by calling IsEnabled.
    case GL_BLEND:
    case GL_CULL_FACE:
    case GL_DEPTH_TEST:
    case GL_DITHER:
    case GL_POLYGON_OFFSET_FILL:
    case GL_SAMPLE_ALPHA_TO_COVERAGE:
    case GL_SAMPLE_COVERAGE:
    case GL_SCISSOR_TEST:
case GL_STENCIL_TEST:
      Convert(data, IsEnabled(value));
      return true;

    // This state is GLES1 state we must emulate..
    case GL_MAX_CLIP_PLANES:
      Convert(data, static_cast<GLint>(UniformContext::kMaxClipPlanes));
      return true;

    case GL_MAX_LIGHTS:
      Convert(data, static_cast<GLint>(UniformContext::kMaxLights));
      return true;

    // TODO(crbug.com/441923): We should use the underlying implementation
    // values for GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS and
    // GL_MAX_TEXTURE_IMAGE_UNITS.
    case GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS:
    case GL_MAX_TEXTURE_IMAGE_UNITS:
    case GL_MAX_TEXTURE_UNITS:
      Convert(data, static_cast<GLint>(UniformContext::kMaxTextureUnits));
      return true;

    // palette matrix relared values:
    case GL_CURRENT_PALETTE_MATRIX_OES:
      Convert(data, uniform_context_.GetCurrentPaletteMatrix());
      return true;
    case GL_MAX_PALETTE_MATRICES_OES:
      Convert(data, UniformContext::kMaxPaletteMatricesOES);
      return true;
    case GL_MAX_VERTEX_UNITS_OES:
      Convert(data, UniformContext::kMaxVertexUnitsOES);
      return true;

    case GL_MATRIX_MODE:
      Convert(data, static_cast<GLuint>(uniform_context_.GetMatrixMode()));
      return true;

    case GL_MODELVIEW_MATRIX: {
        GLfloat tmp[emugl::Matrix::kEntries];
        uniform_context_.GetModelViewMatrix().GetColumnMajorArray(
            tmp, emugl::Matrix::kEntries);
        Convert(data, tmp);
        return true;
      }

    case GL_PROJECTION_MATRIX: {
        GLfloat tmp[emugl::Matrix::kEntries];
        uniform_context_.GetProjectionMatrix().GetColumnMajorArray(
            tmp, emugl::Matrix::kEntries);
        Convert(data, tmp);
        return true;
      }

    case GL_TEXTURE_MATRIX: {
        GLfloat tmp[emugl::Matrix::kEntries];
        uniform_context_.GetTextureMatrix().GetColumnMajorArray(
            tmp, emugl::Matrix::kEntries);
        Convert(data, tmp);
        return true;
      }

    case GL_SMOOTH_POINT_SIZE_RANGE:
      Convert(data, aliased_point_size_range_.Get());
      return true;

    case GL_CLIENT_ACTIVE_TEXTURE:
      Convert(data, texture_context_.GetClientActiveTexture());
      return true;

    // This state is GLES2 state that is cached purely for performance reasons.
    // Note: C++11 will allow us to eliminate duplicate knowledge of the
    // mapping by being able to the pattern:
    //   case decltype(foo::kValue):
    //     Convert(data, foo.Get());
    case GL_ACTIVE_TEXTURE:
      Convert(data, texture_context_.GetActiveTexture());
      return true;

    case GL_ALIASED_LINE_WIDTH_RANGE:
      Convert(data, aliased_line_width_range_.Get());
      return true;

    case GL_ALIASED_POINT_SIZE_RANGE:
      Convert(data, aliased_point_size_range_.Get());
      return true;

    case GL_ARRAY_BUFFER_BINDING:
      Convert(data, array_buffer_binding_);
      return true;

    case GL_BLEND_DST_ALPHA:
      Convert(data, blend_func_dst_alpha_.Get());
      return true;

    case GL_BLEND_DST_RGB:
      Convert(data, blend_func_dst_rgb_.Get());
      return true;

    case GL_BLEND_SRC_ALPHA:
      Convert(data, blend_func_src_alpha_.Get());
      return true;

    case GL_BLEND_SRC_RGB:
      Convert(data, blend_func_src_rgb_.Get());
      return true;

    case GL_COLOR_CLEAR_VALUE:
      Convert(data, color_clear_value_.Get());
      return true;

    case GL_COLOR_WRITEMASK:
      Convert(data, color_writemask_.Get());
      return true;

    case GL_CULL_FACE_MODE:
      Convert(data, cull_face_mode_.Get());
      return true;

    case GL_CURRENT_PROGRAM:
      if (current_user_program_ != NULL) {
        Convert(data, static_cast<GLint>(
            current_user_program_->GetLocalName()));
      } else {
        *data = 0;
      }
      return true;

    case GL_DEPTH_CLEAR_VALUE:
      Convert(data, depth_clear_value_.Get());
      return true;

    case GL_DEPTH_FUNC:
      Convert(data, depth_func_.Get());
      return true;

    case GL_DEPTH_RANGE:
      Convert(data, depth_range_.Get());
      return true;

    case GL_DEPTH_WRITEMASK:
      Convert(data, depth_writemask_.Get());
      return true;

    case GL_ELEMENT_ARRAY_BUFFER_BINDING:
      Convert(data, element_buffer_binding_);
      return true;

    case GL_FRAMEBUFFER_BINDING:
      Convert(data, framebuffer_binding_);
      return true;

    case GL_FRONT_FACE:
      Convert(data, front_face_.Get());
      return true;

    case GL_IMPLEMENTATION_COLOR_READ_TYPE_OES:
      Convert(data, GL_UNSIGNED_BYTE);
      return true;

    case GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES:
      Convert(data, GL_RGBA);
      return true;

    case GL_LINE_WIDTH:
      Convert(data, line_width_.Get());
      return true;

    case GL_GENERATE_MIPMAP_HINT:
      Convert(data, generate_mipmap_hint_.Get());
      return true;

    case GL_MAX_CUBE_MAP_TEXTURE_SIZE:
      Convert(data, max_cube_map_texture_size_.Get());
      return true;

    case GL_MAX_RENDERBUFFER_SIZE:
      Convert(data, max_renderbuffer_size_.Get());
      return true;

    case GL_MAX_TEXTURE_SIZE:
      Convert(data, max_texture_size_.Get());
      return true;

    case GL_MAX_VERTEX_ATTRIBS:
      Convert(data, max_vertex_attribs_.Get());
      return true;

    case GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS:
      Convert(data, max_vertex_texture_image_units_.Get());
      return true;

    case GL_MAX_VIEWPORT_DIMS:
      Convert(data, max_viewport_dims_.Get());
      return true;

    case GL_PACK_ALIGNMENT:
      Convert(data, pixel_store_pack_alignment_.Get());
      return true;

    case GL_POLYGON_OFFSET_FACTOR:
      Convert(data, polygon_offset_factor_.Get());
      return true;

    case GL_POLYGON_OFFSET_UNITS:
      Convert(data, polygon_offset_units_.Get());
      return true;

    case GL_RENDERBUFFER_BINDING:
      Convert(data, renderbuffer_binding_);
      return true;

    case GL_SAMPLE_COVERAGE_VALUE:
      Convert(data, sample_coverage_value_.Get());
      return true;

    case GL_SAMPLE_COVERAGE_INVERT:
      Convert(data, sample_coverage_invert_.Get());
      return true;

    case GL_SCISSOR_BOX:
      Convert(data, scissor_box_.Get());
      return true;

    case GL_STENCIL_FUNC:
      Convert(data, stencil_func_.Get());
      return true;

    case GL_STENCIL_REF:
      Convert(data, stencil_ref_.Get());
      return true;

    case GL_STENCIL_VALUE_MASK:
      Convert(data, stencil_value_mask_.Get());
      return true;

    case GL_TEXTURE_BINDING_CUBE_MAP:
      Convert(data, texture_context_.GetBoundTexture(GL_TEXTURE_CUBE_MAP));
      return true;

    case GL_TEXTURE_BINDING_2D:
      Convert(data, texture_context_.GetBoundTexture(GL_TEXTURE_2D));
      return true;

    case GL_TEXTURE_BINDING_EXTERNAL_OES:
      Convert(data, texture_context_.GetBoundTexture(GL_TEXTURE_EXTERNAL_OES));
      return true;

    case GL_UNPACK_ALIGNMENT:
      Convert(data, pixel_store_unpack_alignment_.Get());
      return true;

    case GL_VIEWPORT:
      Convert(data, viewport_.Get());
      return true;

    // The compressed texture format state is special.
    // GLES1 mandates that ten specific formats must be supported.
    // GLES2 makes support entirely optional.
    case GL_NUM_COMPRESSED_TEXTURE_FORMATS:
      EnsureCompressedTextureFormatStateKnown();
      Convert(data, static_cast<GLint>(supported_compressed_texture_formats_.size()));
      return true;

    case GL_COMPRESSED_TEXTURE_FORMATS:
      EnsureCompressedTextureFormatStateKnown();
      Convert(data, supported_compressed_texture_formats_.begin(),
              supported_compressed_texture_formats_.end());
      return true;
  }

  const PointerData* ptr = NULL;
  switch (value) {
    case GL_VERTEX_ARRAY_BUFFER_BINDING:
    case GL_VERTEX_ARRAY_SIZE:
    case GL_VERTEX_ARRAY_STRIDE:
    case GL_VERTEX_ARRAY_TYPE:
      ptr = pointer_context_.GetPointerData(kPositionVertexAttribute);
      break;
    case GL_NORMAL_ARRAY_BUFFER_BINDING:
    case GL_NORMAL_ARRAY_STRIDE:
    case GL_NORMAL_ARRAY_TYPE:
      ptr = pointer_context_.GetPointerData(kNormalVertexAttribute);
      break;
    case GL_COLOR_ARRAY_BUFFER_BINDING:
    case GL_COLOR_ARRAY_SIZE:
    case GL_COLOR_ARRAY_STRIDE:
    case GL_COLOR_ARRAY_TYPE:
      ptr = pointer_context_.GetPointerData(kColorVertexAttribute);
      break;
    case GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING:
    case GL_TEXTURE_COORD_ARRAY_SIZE:
    case GL_TEXTURE_COORD_ARRAY_STRIDE:
    case GL_TEXTURE_COORD_ARRAY_TYPE:
      ptr = pointer_context_.GetPointerData(
          texture_context_.GetClientActiveTextureCoordAttrib());
      break;
    case GL_POINT_SIZE_ARRAY_BUFFER_BINDING_OES:
    case GL_POINT_SIZE_ARRAY_STRIDE_OES:
    case GL_POINT_SIZE_ARRAY_TYPE_OES:
      ptr = pointer_context_.GetPointerData(kPointSizeVertexAttribute);
      break;
    case GL_WEIGHT_ARRAY_BUFFER_BINDING_OES:
    case GL_WEIGHT_ARRAY_SIZE_OES:
    case GL_WEIGHT_ARRAY_STRIDE_OES:
    case GL_WEIGHT_ARRAY_TYPE_OES:
      ptr = pointer_context_.GetPointerData(kWeightVertexAttribute);
      break;
    case GL_MATRIX_INDEX_ARRAY_BUFFER_BINDING_OES:
    case GL_MATRIX_INDEX_ARRAY_SIZE_OES:
    case GL_MATRIX_INDEX_ARRAY_STRIDE_OES:
    case GL_MATRIX_INDEX_ARRAY_TYPE_OES:
      ptr = pointer_context_.GetPointerData(kMatrixIndexVertexAttribute);
      break;
    default:
      return false;
  }

  switch (value) {
    case GL_VERTEX_ARRAY_BUFFER_BINDING:
    case GL_NORMAL_ARRAY_BUFFER_BINDING:
    case GL_COLOR_ARRAY_BUFFER_BINDING:
    case GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING:
    case GL_POINT_SIZE_ARRAY_BUFFER_BINDING_OES:
    case GL_WEIGHT_ARRAY_BUFFER_BINDING_OES:
    case GL_MATRIX_INDEX_ARRAY_BUFFER_BINDING_OES:
      Convert(data, ptr ? ptr->buffer_name : 0);
      return true;

    case GL_VERTEX_ARRAY_STRIDE:
    case GL_NORMAL_ARRAY_STRIDE:
    case GL_COLOR_ARRAY_STRIDE:
    case GL_TEXTURE_COORD_ARRAY_STRIDE:
    case GL_POINT_SIZE_ARRAY_STRIDE_OES:
    case GL_WEIGHT_ARRAY_STRIDE_OES:
    case GL_MATRIX_INDEX_ARRAY_STRIDE_OES:
      Convert(data, ptr ? ptr->stride : 0);
      return true;

    case GL_VERTEX_ARRAY_SIZE:
    case GL_COLOR_ARRAY_SIZE:
    case GL_TEXTURE_COORD_ARRAY_SIZE:
    case GL_WEIGHT_ARRAY_SIZE_OES:
    case GL_MATRIX_INDEX_ARRAY_SIZE_OES:
      Convert(data, ptr ? ptr->size : 0);
      return true;

    case GL_VERTEX_ARRAY_TYPE:
    case GL_NORMAL_ARRAY_TYPE:
    case GL_COLOR_ARRAY_TYPE:
    case GL_TEXTURE_COORD_ARRAY_TYPE:
    case GL_POINT_SIZE_ARRAY_TYPE_OES:
    case GL_WEIGHT_ARRAY_TYPE_OES:
    case GL_MATRIX_INDEX_ARRAY_TYPE_OES:
      Convert(data, ptr ? ptr->type : 0);
      return true;
  }

  LOG_ALWAYS_FATAL("%s: Attempt to get %s (0x%x)", __FUNCTION__,
                   GetEnumString(value), value);
  return false;
}

bool GlesContext::GetBooleanv(GLenum pname, GLboolean* data) const {
  return GetValue(pname, data);
}

bool GlesContext::GetFloatv(GLenum pname, GLfloat* data) const {
  return GetValue(pname, data);
}

bool GlesContext::GetIntegerv(GLenum pname, GLint* data) const {
  return GetValue(pname, data);
}

bool GlesContext::GetFixedv(GLenum pname, GLfixed* data) const {
  return GetValue(pname, data);
}

const GLubyte* GlesContext::GetString(GLenum pname) const {
  const char* str = NULL;
  switch (pname) {
    case GL_VENDOR:
      str = "Chromium";
      break;
    case GL_RENDERER:
      str = "Chromium";
      break;
    case GL_VERSION:
      if (version_ == kGles11) {
        str = "OpenGL ES 1.1 Chromium";
      } else {
        str = "OpenGL ES 2.0 Chromium";
      }
      break;
    case GL_EXTENSIONS:
      if (version_ == kGles11) {
        str = "GL_OES_EGL_image "
              "GL_OES_blend_equation_separate "
              "GL_OES_blend_func_separate "
              "GL_OES_blend_subtract "
              "GL_OES_byte_coordinates "
              "GL_OES_compressed_ETC1_RGB8_texture "
              "GL_OES_depth24 "
              "GL_OES_depth32 "
              "GL_OES_draw_texture "
              "GL_OES_element_index_uint "
              "GL_OES_framebuffer_object "
              "GL_OES_matrix_palette "
              "GL_EXT_packed_depth_stencil "
              "GL_OES_point_size_array "
              "GL_OES_point_sprite "
              "GL_OES_rgb8_rgba8 "
              "GL_OES_single_precision "
              "GL_OES_stencil1 "
              "GL_OES_stencil4 "
              "GL_OES_stencil8 "
              "GL_OES_stencil_wrap "
              "GL_OES_texture_cube_map "
              "GL_OES_texture_env_crossbar "
              "GL_OES_texture_npot ";
      } else {
        str = "GL_OES_EGL_image "
              "GL_OES_EGL_image_external "
              "GL_OES_compressed_ETC1_RGB8_texture "
              "GL_OES_depth24 "
              "GL_OES_depth32 "
              "GL_OES_depth_texture "
              "GL_OES_element_index_uint "
              "GL_OES_packed_depth_stencil "
              "GL_OES_texture_npot ";
      }
      break;
    case GL_SHADING_LANGUAGE_VERSION:
      str = "OpenGL ES GLSL ES 1.0 Chromium";
      break;
    default:
      LOG_ALWAYS_FATAL("Unsupported string parameter: %s (0x%x)",
                       GetEnumString(pname), pname);
  }
  return reinterpret_cast<const GLubyte*>(str);
}
