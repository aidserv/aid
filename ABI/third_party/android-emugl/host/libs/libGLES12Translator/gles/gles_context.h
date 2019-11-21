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
#ifndef GRAPHICS_TRANSLATION_GLES_GLES_CONTEXT_H_
#define GRAPHICS_TRANSLATION_GLES_GLES_CONTEXT_H_

#include <GLES/gl.h>
#include <GLES2/gl2.h>
#include <map>
#include <set>
#include <string>
#include <utils/RefBase.h>
#include <vector>

#include "gles/egl_image.h"
#include "gles/gles1_shader_generator.h"
#include "gles/gles_utils.h"
#include "gles/mru_cache.h"
#include "gles/share_group.h"
#include "gles/state.h"
#include "gles/texture_data.h"

#include "GLES12Translator/underlying_apis.h"

class GlesContext {
 public:
  enum DrawType {
    kDrawArrays,
    kDrawElements,
  };

  // Ensures that the bound surface has a buffer ready for drawing.
  class SurfaceControlCallback : public android::RefBase {
   public:
    virtual void EnsureBufferReady() = 0;
   protected:
    virtual ~SurfaceControlCallback() {}
  };
  typedef android::sp<SurfaceControlCallback> SurfaceControlCallbackPtr;

  GlesContext(int32_t id, GlesVersion ver, GlesContext* share,
              void* underlying_context, const UnderlyingApis* underlying_apis);
  ~GlesContext();

  void* Impl() const { return gles_impl_; }
  const UnderlyingApis* Apis() const { return apis_; }
  GlesVersion GetVersion() const { return version_; }
  int32_t GetId() const { return id_; }

  ShareGroupPtr GetShareGroup() const;

  void SetGLerror(GLenum error, const char* function, int line_number,
                  const char* fmt, ...);
  GLenum GetGLerror();

  bool AreChecksEnabled() const;

  void OnMakeCurrent();
  void OnAttachSurface(SurfaceControlCallbackPtr sfc, int width, int height);
  void EnsureSurfaceReadyToDraw() const;
  void Flush();

  void UpdateFramebufferOverride(GLint width, GLint draw_height,
                                 GLint depth_size, GLint stencil_size,
                                 GLuint global_texture_name);

  void DrawFullscreenQuad(GLuint texture, bool flip_v);
  GLuint CompileShader(GLenum shader_kind, const char* source);
  GLuint CompileProgram(GLuint vertex_shader, GLuint fragment_shader);

  // Get data stored in this context.
  bool IsEnabled(GLenum cap) const { return enabled_set_.count(cap) > 0; }

  bool GetFloatv(GLenum pname, GLfloat* params) const;
  bool GetFixedv(GLenum pname, GLfixed* params) const;
  bool GetIntegerv(GLenum pname, GLint* params) const;
  bool GetBooleanv(GLenum pname, GLboolean* params) const;
  const GLubyte* GetString(GLenum pname) const;

  // Data accessors for bound objects.
  BufferDataPtr GetBoundTargetBufferData(GLenum target);
  TextureDataPtr GetBoundTextureData(GLenum target);
  FramebufferDataPtr GetBoundFramebufferData();
  RenderbufferDataPtr GetBoundRenderbufferData();

  // Draw functions.
  void Draw(DrawType draw, GLenum mode, GLint first, GLsizei count,
            GLenum type, const GLvoid* indices);

  // EGL image functions
  bool BindImageToTexture(GLenum target, EglImagePtr image);
  bool BindImageToRenderbuffer(EglImagePtr image);

  bool IsCompressedFormatEmulationNeeded(GLenum format) const;

  // Framebuffer functions
  void BindFramebuffer(GLuint framebuffer);

  // DrawTex function
  void DrawTex(GLfloat x, GLfloat y, GLfloat z, GLfloat width, GLfloat height);

  void Invalidate();
  void Restore();

  // Classes to handle specific contexts.
  UniformContext uniform_context_;
  TextureContext texture_context_;
  PointerContext pointer_context_;

  // Set of GL flags that are currently enabled.
  std::set<GLenum> enabled_set_;

  GLuint array_buffer_binding_;
  GLuint element_buffer_binding_;
  GLuint framebuffer_binding_;
  GLuint renderbuffer_binding_;

  // Returns application's program set by glUseProgram().
  ProgramDataPtr GetCurrentUserProgram() {
    return current_user_program_;
  }
  void SetCurrentUserProgram(const ProgramDataPtr& program);

  // TODO(crbug.com/426083): Some of this state is read-only, and can be lazily
  // loaded once for all contexts, since the values depend only on
  // implementation details of the underlying GLES2 implementation.
  // Included would be these first two _RANGE values, as well as the MAX_ values
  // further down.
  StateCache<GLfloat[2], GL_ALIASED_LINE_WIDTH_RANGE> aliased_line_width_range_;
  StateCache<GLfloat[2], GL_ALIASED_POINT_SIZE_RANGE> aliased_point_size_range_;
  StateCache<GLint, GL_BLEND_DST_ALPHA> blend_func_dst_alpha_;
  StateCache<GLint, GL_BLEND_DST_RGB> blend_func_dst_rgb_;
  StateCache<GLint, GL_BLEND_SRC_ALPHA> blend_func_src_alpha_;
  StateCache<GLint, GL_BLEND_SRC_RGB> blend_func_src_rgb_;
  StateCache<GLfloat[4], GL_COLOR_CLEAR_VALUE> color_clear_value_;
  StateCache<GLboolean[4], GL_COLOR_WRITEMASK> color_writemask_;
  StateCache<GLint, GL_CULL_FACE_MODE> cull_face_mode_;
  StateCache<GLfloat, GL_DEPTH_CLEAR_VALUE> depth_clear_value_;
  StateCache<GLint, GL_DEPTH_FUNC> depth_func_;
  StateCache<GLfloat[2], GL_DEPTH_RANGE> depth_range_;
  StateCache<GLboolean, GL_DEPTH_WRITEMASK> depth_writemask_;
  StateCache<GLint, GL_FRONT_FACE> front_face_;
  StateCache<GLfloat, GL_LINE_WIDTH> line_width_;
  StateCache<GLint, GL_GENERATE_MIPMAP_HINT> generate_mipmap_hint_;
  StateCache<GLint, GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS>
      max_combined_texture_image_units_;
  StateCache<GLint, GL_MAX_CUBE_MAP_TEXTURE_SIZE> max_cube_map_texture_size_;
  StateCache<GLint, GL_MAX_RENDERBUFFER_SIZE> max_renderbuffer_size_;
  StateCache<GLint, GL_MAX_TEXTURE_IMAGE_UNITS> max_texture_image_units_;
  StateCache<GLint, GL_MAX_TEXTURE_SIZE> max_texture_size_;
  StateCache<GLuint, GL_MAX_VERTEX_ATTRIBS> max_vertex_attribs_;
  StateCache<GLint, GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS>
      max_vertex_texture_image_units_;
  StateCache<GLint[2], GL_MAX_VIEWPORT_DIMS> max_viewport_dims_;
  StateCache<GLint, GL_PACK_ALIGNMENT> pixel_store_pack_alignment_;
  StateCache<GLint, GL_UNPACK_ALIGNMENT> pixel_store_unpack_alignment_;
  StateCache<GLfloat, GL_POLYGON_OFFSET_FACTOR> polygon_offset_factor_;
  StateCache<GLfloat, GL_POLYGON_OFFSET_UNITS> polygon_offset_units_;
  StateCache<GLboolean, GL_SAMPLE_COVERAGE_INVERT> sample_coverage_invert_;
  StateCache<GLint[4], GL_SCISSOR_BOX> scissor_box_;
  StateCache<GLfloat, GL_SAMPLE_COVERAGE_VALUE> sample_coverage_value_;
  StateCache<GLint, GL_STENCIL_FUNC> stencil_func_;
  StateCache<GLint, GL_STENCIL_VALUE_MASK> stencil_value_mask_;
  StateCache<GLint, GL_STENCIL_REF> stencil_ref_;
  StateCache<GLint[4], GL_VIEWPORT> viewport_;

  StateCache<GLint, GL_ALPHA_BITS> default_framebuffer_alpha_bits_;
  StateCache<GLint, GL_BLUE_BITS> default_framebuffer_blue_bits_;
  StateCache<GLint, GL_DEPTH_BITS> default_framebuffer_depth_bits_;
  StateCache<GLint, GL_GREEN_BITS> default_framebuffer_green_bits_;
  StateCache<GLint, GL_RED_BITS> default_framebuffer_red_bits_;
  StateCache<GLint, GL_STENCIL_BITS> default_framebuffer_stencil_bits_;
  StateCache<GLint, GL_SAMPLE_BUFFERS> default_framebuffer_sample_buffers_;

 private:
  static const int kCacheLimit = 16;
  static const int kErrorLimit = 16;
  static const int kMaximumErrorCounterSize = 2064;

  typedef MruCache<ShaderConfig, GLuint> ShaderCache;
  typedef MruCache<ShaderConfig, ProgramContext> ProgramCache;
  typedef std::vector<GLenum> TextureFormats;

  template <typename T>
  bool GetValue(GLenum value, T* data) const;

  bool CanDraw() const;

  GLenum GetEnabledTextureTarget(GLenum id) const;

  void PrepareProgramObject(GLenum mode, bool* program_uses_external_as_2d);
  void ClearProgramObject();

  ProgramContext& BindProgramContext(GLenum mode);
  ShaderConfig ConfigureShader(GLenum mode);

  void EnsureCompressedTextureFormatStateKnown() const;
  void EnsureUnderlyingExtensionsKnown() const;

  void SetupColorTextureAttachment(GLuint texture);
  void SetupDepthStencilAttachment(GLint width, GLint height,
                                   GLint depth_size, GLint stencil_size);
  void DeleteFramebufferOverride();

  GlesVersion version_;
  const int32_t id_;
  bool initialized_;
  bool initialized_viewport_;
  GLenum error_;
  std::map<uint32_t, uint32_t> error_counter_;

  void* gles_impl_;
  const UnderlyingApis* apis_;
  ShareGroupPtr share_group_;

  // This is the default framebuffer object to use in place of framebuffer zero.
  GLuint global_override_framebuffer_;

  // This state is used to maintain a framebuffer for rendering based on the
  // last attached EGL surface propoperties.
  GLuint global_framebuffer_;
  GLuint global_depth_renderbuffer_;
  GLuint global_stencil_renderbuffer_;
  GLint cached_draw_width_;
  GLint cached_draw_height_;
  GLint cached_draw_stencil_size_;
  GLint cached_draw_depth_size_;
  GLuint cached_global_texture_;

  ShaderCache vertex_shader_cache_;
  ShaderCache fragment_shader_cache_;
  ProgramCache program_cache_;
  FullscreenQuad* fullscreen_quad_;
  ProgramDataPtr current_user_program_;
  SurfaceControlCallbackPtr surface_callback_;

  // This is the set of supported compressed texture formats given to the
  // client.
  mutable TextureFormats supported_compressed_texture_formats_;
  // This is the set of compressed texture formats which need to be emulated as
  // there is no support in the underlying implementation.
  mutable TextureFormats emulated_compressed_texture_formats_;

  bool checks_enabled_;
  mutable const char* global_extensions_;
  mutable bool supports_packed_depth_stencil_;

  GlesContext(const GlesContext&);
  GlesContext& operator=(const GlesContext&);
};

#endif  // GRAPHICS_TRANSLATION_GLES_GLES_CONTEXT_H_
