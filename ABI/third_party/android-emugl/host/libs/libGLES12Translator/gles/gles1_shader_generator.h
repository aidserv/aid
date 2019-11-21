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
#ifndef GRAPHICS_TRANSLATION_GLES_GLES1_SHADER_GENERATOR_H_
#define GRAPHICS_TRANSLATION_GLES_GLES1_SHADER_GENERATOR_H_

#include <GLES/gl.h>
#include "gles/state.h"

struct ShaderConfig {
  ShaderConfig() { memset(this, 0, sizeof(*this)); }

  // Since we use memcmp in operator==, we need to make sure we copy all the
  // bits including any padding.  (The default copy constructor will ignore
  // any padding bits.)
  ShaderConfig(const ShaderConfig& rhs) { memcpy(this, &rhs, sizeof(rhs)); }

  bool operator==(const ShaderConfig& rhs) const {
    return memcmp(this, &rhs, sizeof(rhs)) == 0;
  }

  // TODO(crbug.com/441920): This structure duplicates the TexEnv & TexGen
  // structures somewhat.  It would be nice to eliminate that duplication.
  struct TextureConfig {
    bool enabled;
    GLenum mode;
    GLenum target;
    GLenum format;
    GLenum combine_rgb;
    GLenum combine_alpha;
    GLenum src_rgb[3];
    GLenum src_alpha[3];
    GLenum operand_rgb[3];
    GLenum operand_alpha[3];
    GLenum texgen_mode;
    bool texgen_enabled;
  };

  struct LightConfig {
    bool enabled;
    bool directional;
    bool spot;
    bool attenuate;
  };

  TextureConfig texture[UniformContext::kMaxTextureUnits];
  LightConfig light[UniformContext::kMaxLights];
  GLenum mode;
  GLenum fog_mode;
  GLenum alpha_func;
  GLuint vertex_units;
  bool enable_clip_plane[UniformContext::kMaxClipPlanes];
  bool enable_alpha_test : 1;
  bool enable_color_material : 1;
  bool enable_fog : 1;
  bool enable_light_model_two_side : 1;
  bool enable_lighting : 1;
  bool enable_normalize : 1;
  bool enable_point_smooth : 1;
  bool enable_point_sprite : 1;
  bool enable_matrix_palette : 1;
  bool any_clip_planes_enabled : 1;
  bool any_texture_units_enabled : 1;
  bool any_texture_gen_normal_map : 1;
  bool any_texture_gen_reflection_map : 1;

 private:
  ShaderConfig& operator=(const ShaderConfig&);
};

void GenerateVertexShader(const ShaderConfig& cfg, char* buffer, size_t len);

void GenerateFragmentShader(const ShaderConfig& cfg, char* buffer, size_t len);

#endif  // GRAPHICS_TRANSLATION_GLES_GLES1_SHADER_GENERATOR_H_
