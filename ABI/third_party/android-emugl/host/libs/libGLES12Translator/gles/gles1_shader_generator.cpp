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

#include "gles/gles1_shader_generator.h"

#include <stdio.h>
#include <string.h>
#include <GLES/glext.h>

#include "common/alog.h"
#include "gles/debug.h"

#if defined(COMPILER_GCC)
#define PRINTF_FORMAT(format_param, dots_param) \
    __attribute__((format(printf, format_param, dots_param)))
#else
#define PRINTF_FORMAT(format_param, dots_param)
#endif

// Generates a GLES1 equivalent shader for use in GLES2
// Unless otherwise specified, all references to the GLES1 specification here
// are meant to be against the Khronos OpenGL 1.1.12 (April 2008) PDF file.
// (http://www.khronos.org/registry/gles/specs/1.1/es_full_spec.1.1.12.pdf)

// The following conventions are used in the shader code.
// uniform variables: Prepended with u_.  These are constants available to both
//     the vertex and fragment shaders.
// attribute variables: Prepended with a_.  These are vertex shader only data.
// varying variables: Prepended with v_.  Data passed from the vertex shader to
//    to the fragment shader data, subject to interpolation.
// gl vertex/fragment output data:   Prepended with gl_.  These are defined by
//    the GLSL itself.

namespace {

class StringBuilder {
 public:
  StringBuilder(char* buf, size_t size) : buf_(buf), size_(size) {
    LOG_ALWAYS_FATAL_IF(buf == NULL || size_ == 0);
    // Always ensure the buffer is nul terminated.
    *buf_ = 0;
    // Reduce available size by 1 because we'll always add a nul terminator
    // to the end of the buffer.
    --size_;
  }

  void Write(const char* text) {
    size_t len = strlen(text);
    LOG_ALWAYS_FATAL_IF(size_ < len);
    if (len > size_) {
      len = size_;
    }

    memcpy(buf_, text, len);

    buf_ += len;
    size_ -= len;
    EndLine('\n');
  }

  void Format(const char* format, ...) PRINTF_FORMAT(2, 3) {
    va_list args;
    va_start(args, format);
    size_t len = vsnprintf(buf_, size_, format, args);
    va_end(args);

    LOG_ALWAYS_FATAL_IF(size_ < len);
    if (len > size_) {
      len = size_;
    }

    buf_ += len;
    size_ -= len;
    EndLine('\n');
  }

 private:
  void EndLine(const char ch) {
    LOG_ALWAYS_FATAL_IF(size_ < 2);
    if (size_ > 1) {
      *buf_ = '\n';
      ++buf_;
      --size_;

      // Always ensure the buffer is nul terminated.
      *buf_ = 0;
    }
  }

  char* buf_;
  size_t size_;
};

bool NeedsBackColor(const ShaderConfig& cfg) {
  return cfg.enable_lighting && cfg.enable_light_model_two_side;
}

bool NeedsNormal(const ShaderConfig& cfg) {
  return (cfg.enable_lighting || cfg.any_texture_gen_normal_map ||
          cfg.any_texture_gen_reflection_map);
}

void WriteVSHeader(const ShaderConfig& cfg, StringBuilder* b) {
  b->Write("#version 100");

  if (cfg.enable_lighting) {
    b->Format("#define NUM_LIGHTS %d", UniformContext::kMaxLights);

    b->Write("struct light {");
    b->Write("  vec4 ambient;");
    b->Write("  vec4 diffuse;");
    b->Write("  vec4 specular;");
    b->Write("  vec4 position;");
    b->Write("  vec4 direction;");
    b->Write("  float spot_exponent;");
    b->Write("  float spot_cutoff;");
    b->Write("  float const_attenuation;");
    b->Write("  float linear_attenuation;");
    b->Write("  float quadratic_attenuation;");
    b->Write("};");

    b->Write("struct material {");
    b->Write("  vec4 ambient;");
    b->Write("  vec4 diffuse;");
    b->Write("  vec4 specular;");
    b->Write("  vec4 emission;");
    b->Write("  float shininess;");
    b->Write("};");

    // See es_full_spec.1.1.12.pdf section 2.12.1 equation 2.4.
    // vp: Vector from vertex position (V) to light position (P).
    b->Write("float lightAttenuation(light in_light, vec3 vp) {");
    b->Write("  vec3 att = vec3(in_light.const_attenuation,");
    b->Write("                  in_light.linear_attenuation,");
    b->Write("                  in_light.quadratic_attenuation);");
    b->Write("  float len = length(vp);");
    b->Write("  return 1.0 / dot( att, vec3(1.0, len, len * len));");
    b->Write("}");

    // See es_full_spec.1.1.12.pdf section 2.12.1 equation 2.5.
    // vp: Unit vector from vertex position (V) to light position (P).
    // exp(A * log(B)) is equivalent to pow(B, A).
    b->Write("float lightSpotShininess(light in_light, vec3 vp) {");
    b->Write("  float sd = dot(vp, in_light.direction.xyz);");
    b->Write("  return (sd >= in_light.spot_cutoff) ?");
    b->Write("         exp(in_light.spot_exponent * log(sd)) :");
    b->Write("         0.0;");
    b->Write("}");

    // See es_full_spec.1.1.12.pdf section 2.12.1, equation 2.1.
    // vp: Vector from vertex position (V) to light position (P).
    // norm: Vertex normal.
    // att: Attenuation factor (calculated externally).
    // spot: Spot light factor (calculated externally).
    // hv: See equation 2.3.
    // df: Diffuse factor.
    // sf: Specular factor.  See equation 2.2.
    b->Write("vec4 applyLight(light in_light, material in_material, vec3 vp,");
    b->Write("                vec3 norm, float att, float spot) {");
    b->Write("  vec3 hv = normalize(vp + vec3(0.0, 0.0, 1.0));");
    b->Write("  float df = max(dot(norm, vp), 0.0);");
    b->Write("  float sf = max(dot(norm, hv), 0.0);");
    b->Write("  sf = (df > 0.0 && sf > 0.0)");
    b->Write("     ? exp(in_material.shininess * log(sf))");
    b->Write("     : 0.0;");
    b->Write("  vec4 ambient = in_light.ambient * in_material.ambient;");
    b->Write("  vec4 diffuse = df * in_light.diffuse * in_material.diffuse;");
    b->Write("  vec4 specular = ");
    b->Write("      sf * in_light.specular * in_material.specular;");
    b->Write("  vec4 color = att * spot * (ambient + diffuse + specular);");
    b->Write("  return color;");
    b->Write("}");

    b->Write("uniform mediump vec4 u_ambient;");
    b->Write("uniform material u_material;");
    b->Write("uniform light u_light[NUM_LIGHTS];");
  }

  if (cfg.enable_fog) {
    b->Write("varying mediump float v_fog_distance;");
  }

  if (cfg.mode == GL_POINTS) {
    // Every point can get a unique size along with a position.
    b->Write("attribute mediump float a_point_size;");
    // These are the constants used when rendering a set of points.
    b->Write("uniform mediump float u_point_size_min;");
    b->Write("uniform mediump float u_point_size_max;");
    b->Write("uniform mediump vec3 u_point_size_attenuation;");
  }

  if (cfg.any_clip_planes_enabled) {
    b->Format("#define NUM_CLIP_PLANES %d", UniformContext::kMaxClipPlanes);
    b->Write("uniform highp vec4 u_clip_plane[NUM_CLIP_PLANES];");
    b->Write("varying highp float v_clip_plane_distance[NUM_CLIP_PLANES];");
  }

  if (cfg.any_texture_units_enabled) {
    b->Format("#define NUM_TEXTURE_STAGES %d",
              UniformContext::kMaxTextureUnits);
    b->Write("uniform mediump mat4 u_texture_matrix[NUM_TEXTURE_STAGES];");
    b->Write("varying mediump vec4 v_texcoord[NUM_TEXTURE_STAGES];");
    // Attributes cannot be arrays, so create them explicitly
    for (int stage = 0; stage < UniformContext::kMaxTextureUnits; ++stage) {
      if (cfg.texture[stage].enabled) {
        b->Format("attribute mediump vec4 a_texcoord_%d;", stage);
      }
    }
  }

  if (NeedsNormal(cfg)) {
    b->Write("attribute highp vec3 a_normal;");
    b->Write("uniform mediump mat4 u_mv_inverse_transpose_matrix;");
  }

  if (NeedsBackColor(cfg)) {
    b->Write("varying mediump vec4 v_back_color;");
  }

  if (cfg.enable_matrix_palette) {
    b->Format("#define NUM_PALETTE_MATRICES %d",
              UniformContext::kMaxPaletteMatricesOES);
    b->Write("attribute mediump vec4 a_weight;");
    b->Write("attribute mediump vec4 a_matrix_index;");
    b->Write("uniform mediump mat4 u_palette_matrix[NUM_PALETTE_MATRICES];");
    if (NeedsNormal(cfg)) {
      b->Write("uniform mediump mat4 u_palette_inv_matrix[NUM_PALETTE_MATRICES];");
    }
  }

  b->Write("varying mediump vec4 v_front_color;");

  b->Write("uniform mediump mat4 u_p_matrix;");
  b->Write("uniform mediump mat4 u_mv_matrix;");

  b->Write("attribute mediump vec4 a_color;");
  b->Write("attribute highp vec4 a_position;");
}

void WriteFSHeader(const ShaderConfig& cfg, StringBuilder* b) {
  b->Write("#version 100");

  if (cfg.enable_alpha_test) {
    b->Write("uniform mediump float u_alpha_test_constant;");
  }

  if (cfg.enable_fog) {
    b->Write("uniform mediump vec4 u_fog_color;");
    b->Write("uniform mediump float u_fog_density;");
    b->Write("uniform mediump float u_fog_start;");
    b->Write("uniform mediump float u_fog_end;");
    b->Write("varying mediump float v_fog_distance;");
  }

  if (cfg.any_clip_planes_enabled) {
    b->Format("#define NUM_CLIP_PLANES %d", UniformContext::kMaxClipPlanes);
    b->Write("varying highp float v_clip_plane_distance[NUM_CLIP_PLANES];");
  }

  if (cfg.any_texture_units_enabled) {
    b->Format("#define NUM_TEXTURE_STAGES %d",
              UniformContext::kMaxTextureUnits);
    b->Write("uniform mediump vec4 u_texenv_const[NUM_TEXTURE_STAGES];");
    b->Write("uniform mediump vec4 u_texenv_scale[NUM_TEXTURE_STAGES];");
    b->Write("varying mediump vec4 v_texcoord[NUM_TEXTURE_STAGES];");

    for (GLint stage = 0; stage < UniformContext::kMaxTextureUnits; ++stage) {
      const ShaderConfig::TextureConfig& tex = cfg.texture[stage];
      if (!tex.enabled) {
        continue;
      }
      if (tex.target == GL_TEXTURE_CUBE_MAP_OES) {
        b->Format("uniform samplerCube u_sampler_%d;", stage);
      } else if (tex.target == GL_TEXTURE_2D) {
        b->Format("uniform sampler2D u_sampler_%d;", stage);
      } else {
        LOG_ALWAYS_FATAL("Unknown texture target %d (%s)", tex.target,
                         GetEnumString(tex.target));
      }
    }
  }

  if (NeedsBackColor(cfg)) {
    b->Write("varying mediump vec4 v_back_color;");
  }

  b->Write("varying mediump vec4 v_front_color;");
}

void WriteVSComputePointSize(const ShaderConfig& cfg, StringBuilder* b) {
  if (cfg.mode != GL_POINTS) {
    return;
  }

  // es_full_spec.1.1.12.pdf section 3.3
  b->Write("float eye_distance = -eye_position.z;");
  b->Write(
      "float attenuation = dot(u_point_size_attenuation, "
      "vec3(1, eye_distance, eye_distance * eye_distance));");
  b->Write("float point_size = a_point_size / sqrt(attenuation);");
  b->Write(
      "gl_PointSize = clamp(point_size, u_point_size_min, u_point_size_max);");
}

void WriteVSComputeClipPlaneDistances(const ShaderConfig& cfg,
                                      StringBuilder* b) {
  // es_full_spec.1.1.12.pdf section 2.11
  for (int i = 0; i < UniformContext::kMaxClipPlanes; ++i) {
    if (cfg.enable_clip_plane[i]) {
      b->Format(
          "v_clip_plane_distance[%d] = dot(eye_position, "
          "u_clip_plane[%d]);", i, i);
    }
  }
}

void WriteFSTestPointShape(const ShaderConfig& cfg, StringBuilder* b) {
  if (cfg.enable_point_smooth && !cfg.enable_point_sprite) {
    b->Format(
        "if (distance(gl_PointCoord, vec2(0.5, 0.5)) > 0.5) { "
        " discard; }");
  }
}

void WriteFSTestClipPlanes(const ShaderConfig& cfg, StringBuilder* b) {
  // es_full_spec.1.1.12.pdf section 2.11
  for (int i = 0; i < UniformContext::kMaxClipPlanes; ++i) {
    if (cfg.enable_clip_plane[i]) {
      b->Format("if (v_clip_plane_distance[%d] <= 0.0) { discard; }", i);
    }
  }
}

void WriteVSComputeTextureCoordinates(const ShaderConfig& cfg,
                                      StringBuilder* b) {
  // es_full_spec.1.1.12.pdf section 2.10
  // The only thing to do with texture coordinates in GLES1 is to transform them
  // by the current texture matrix. The original OpenGL specification includes
  // the ability to generate texture coordinates from the position/normal.

  if (cfg.any_texture_gen_reflection_map) {
    b->Write("vec4 reflection = vec4(reflect(eye_position.xyz, normal), 0);");
  }

  for (int stage = 0; stage < UniformContext::kMaxTextureUnits; ++stage) {
    if (cfg.texture[stage].enabled) {
      if (cfg.texture[stage].texgen_enabled) {
        if (cfg.texture[stage].texgen_mode == GL_NORMAL_MAP_OES) {
          b->Format("v_texcoord[%d] = u_texture_matrix[%d] * vec4(normal, 0);",
                    stage, stage);
        } else if (cfg.texture[stage].texgen_mode == GL_REFLECTION_MAP_OES) {
          b->Format("v_texcoord[%d] = u_texture_matrix[%d] * reflection;",
                    stage, stage);
        } else {
          LOG_ALWAYS_FATAL("Not supported GL_TEXTURE_GEN_MODE %s",
                           GetEnumString(cfg.texture[stage].texgen_mode));
        }
      } else {
        b->Format("v_texcoord[%d] = u_texture_matrix[%d] * a_texcoord_%d;",
                  stage, stage, stage);
      }
    }
  }
}

void WriteVSComputeFogDistance(const ShaderConfig& cfg, StringBuilder* b) {
  if (cfg.enable_fog) {
    b->Write("v_fog_distance = -eye_position.z;");
  }
}

void WriteVSComputeLighting(const ShaderConfig& cfg, StringBuilder* b) {
  if (cfg.enable_lighting) {
    // es_full_spec.1.1.12.pdf section 2.12.1.
    b->Write("material m = u_material;");
    if (cfg.enable_color_material) {
      b->Write("m.ambient = a_color;");
      b->Write("m.diffuse = a_color;");
    }

    b->Write("v_front_color = m.emission;");
    b->Write("v_front_color += m.ambient * u_ambient;");

    if (cfg.enable_light_model_two_side) {
      b->Write("v_back_color = m.emission;");
      b->Write("v_back_color += m.ambient * u_ambient;");
    }

    for (int i = 0; i < UniformContext::kMaxLights; ++i) {
      const ShaderConfig::LightConfig& light = cfg.light[i];
      if (light.enabled) {
          b->Write("{");
          if (light.directional) {
            b->Format("vec3 lv = u_light[%d].position.xyz;", i);
          } else {
            b->Format("vec3 lp = u_light[%d].position.xyz - eye_position.xyz;",
                      i);
            b->Write("vec3 lv = normalize(lp);");
          }
          if (light.spot) {
            b->Format("float spot = lightSpotShininess(u_light[%d], lv);", i);
          } else {
            b->Write("float spot = 1.0;");
          }
          if (light.attenuate) {
            b->Format("float att = lightAttenuation(u_light[%d], lp);", i);
          } else {
            b->Write("float att = 1.0;");
          }
          b->Format(
              "v_front_color += applyLight(u_light[%d], m, lv, normal, "
              "att, spot);", i);

          if (cfg.enable_light_model_two_side) {
            b->Format(
                "v_back_color += applyLight(u_light[%d], m, lv, -normal, "
                "att, spot);",
                i);
          }
          b->Write("}");
      }
    }

    if (cfg.enable_light_model_two_side) {
      b->Write("v_back_color.a = m.diffuse.a;");
      b->Write("v_back_color = clamp(v_back_color, 0.0, 1.0);");
    }

    b->Write("v_front_color.a = m.diffuse.a;");
    b->Write("v_front_color = clamp(v_front_color, 0.0, 1.0);");

  } else {
    b->Write("v_front_color = clamp(a_color, 0.0, 1.0);");
  }
}

void WriteFSSelectFaceColor(const ShaderConfig& cfg, StringBuilder* b) {
  // es_full_spec.1.1.12.pdf section 2.12
  // Lighting computes one (front) or two (front and back) colors.
  // This function selects which one is supposed to be used on the fragment.
  if (NeedsBackColor(cfg)) {
    b->Write("gl_FragColor = gl_FrontFacing ? v_front_color : v_back_color;");
  } else {
    b->Write("gl_FragColor = v_front_color;");
  }
}

bool DoesFormatHaveColorComponents(GLenum format) {
  return format != GL_ALPHA;
}

bool DoesFormatHaveAlphaComponents(GLenum format) {
  return format != GL_LUMINANCE && format != GL_RGB;
}

void WriteFSCompositeReplace(GLenum format, StringBuilder* b) {
  // es_full_spec.1.1.12.pdf table 3.15
  if (DoesFormatHaveColorComponents(format)) {
    b->Write("gl_FragColor.rgb = sampler.rgb;");
  } else {
    b->Write("gl_FragColor.rgb = previous.rgb;");
  }
  if (DoesFormatHaveAlphaComponents(format)) {
    b->Write("gl_FragColor.a = sampler.a;");
  } else {
    b->Write("gl_FragColor.a = previous.a;");
  }
}

void WriteFSCompositeModulate(GLenum format, StringBuilder* b) {
  // es_full_spec.1.1.12.pdf table 3.15
  if (DoesFormatHaveColorComponents(format)) {
    b->Write("gl_FragColor.rgb = previous.rgb * sampler.rgb;");
  } else {
    b->Write("gl_FragColor.rgb = previous.rgb;");
  }
  if (DoesFormatHaveAlphaComponents(format)) {
    b->Write("gl_FragColor.a = previous.a * sampler.a;");
  } else {
    b->Write("gl_FragColor.a = previous.a;");
  }
}

void WriteFSCompositeDecal(GLenum format, StringBuilder* b) {
  // es_full_spec.1.1.12.pdf table 3.15
  if (format == GL_RGBA) {
    b->Write("gl_FragColor.rgb = mix(previous.rgb, sampler.rgb, sampler.a);");
  } else {
    b->Write("gl_FragColor.rgb = sampler.rgb;");
  }
  b->Write("gl_FragColor.a = previous.a;");
}

void WriteFSCompositeBlend(GLenum format, StringBuilder* b) {
  // es_full_spec.1.1.12.pdf table 3.16
  if (DoesFormatHaveColorComponents(format)) {
    b->Write(
        "gl_FragColor.rgb = mix(previous.rgb, constant.rgb, sampler.rgb);");
  } else {
    b->Write("gl_FragColor.rgb = previous.rgb;");
  }
  if (DoesFormatHaveAlphaComponents(format)) {
    b->Write("gl_FragColor.a = previous.a * sampler.a;");
  } else {
    b->Write("gl_FragColor.a = previous.a;");
  }
}

void WriteFSCompositeAdd(GLenum format, StringBuilder* b) {
  // es_full_spec.1.1.12.pdf table 3.16
  if (DoesFormatHaveColorComponents(format)) {
    b->Write("gl_FragColor.rgb = previous.rgb + sampler.rgb;");
  } else {
    b->Write("gl_FragColor.rgb = previous.rgb;");
  }
  if (DoesFormatHaveAlphaComponents(format)) {
    b->Write("gl_FragColor.a = previous.a * sampler.a;");
  } else {
    b->Write("gl_FragColor.a = previous.a;");
  }

  // Note: This clamp is not mentioned for this operation in 3.7.12, but
  // without it the output of this stage would not be a proper color
  // value.
  b->Write("gl_FragColor = clamp(gl_FragColor, kZero, kOne);");
}

const char* GetFSCombineSourceName(int source) {
  switch (source) {
    case GL_PREVIOUS:
      return "previous";
    case GL_PRIMARY_COLOR:
      return "fragment";
    case GL_TEXTURE:
      return "sampler";
    case GL_CONSTANT:
      return "constant";
    default:
      LOG_ALWAYS_FATAL("Unknown source %d (%s)", source, GetEnumString(source));
      return "previous";
  }
}

// Chooses a source color and alpha for each argument in the combine operation.
void WriteFSCombineSourceSetup(const ShaderConfig::TextureConfig& tex,
                               int arg, StringBuilder* b) {
  // es_full_spec.1.1.12.pdf table 3.18.
  const char* color_source = GetFSCombineSourceName(tex.src_rgb[arg]);
  const int color_operand = tex.operand_rgb[arg];

  switch (color_operand) {
    case GL_SRC_COLOR:
      b->Format("arg%d.rgb = %s.rgb;", arg, color_source);
      break;
    case GL_ONE_MINUS_SRC_COLOR:
      b->Format("arg%d.rgb = kOne.rgb - %s.rgb;", arg, color_source);
      break;
    case GL_SRC_ALPHA:
      b->Format("arg%d.rgb = kOne.rgb * %s.a;", arg, color_source);
      break;
    case GL_ONE_MINUS_SRC_ALPHA:
      b->Format("arg%d.rgb = kOne.rgb * (kOne.a - %s.a);", arg, color_source);
      break;
    default:
      LOG_ALWAYS_FATAL("Unknown operand %d (%s)", color_operand,
                       GetEnumString(color_operand));
  }

  const char* alpha_source = GetFSCombineSourceName(tex.src_alpha[arg]);
  const int alpha_operand = tex.operand_alpha[arg];

  // es_full_spec.1.1.12.pdf table 3.19.
  switch (alpha_operand) {
    case GL_SRC_ALPHA:
      b->Format("arg%d.a = %s.a;", arg, alpha_source);
      break;
    case GL_ONE_MINUS_SRC_ALPHA:
      b->Format("arg%d.a = kOne.a - %s.a;", arg, alpha_source);
      break;
    default:
      LOG_ALWAYS_FATAL("Unknown operand %d (%s)", alpha_operand,
                       GetEnumString(alpha_operand));
  }
}

void WriteFSCompositeCombine(const ShaderConfig::TextureConfig& tex,
                             int stage, StringBuilder* b) {
  // Each combine computation can three configurable sources.
  for (int arg = 0; arg < 3; arg++) {
    WriteFSCombineSourceSetup(tex, arg, b);
  }

  const int color_combine_op = tex.combine_rgb;
  const int alpha_combine_op = tex.combine_alpha;

  // es_full_spec.1.1.12.pdf table 3.17.
  switch (color_combine_op) {
    case GL_REPLACE:
      b->Write("gl_FragColor.rgb = arg0.rgb;");
      break;
    case GL_MODULATE:
      b->Write("gl_FragColor.rgb = arg0.rgb * arg1.rgb;");
      break;
    case GL_ADD:
      b->Write("gl_FragColor.rgb = arg0.rgb + arg1.rgb;");
      break;
    case GL_ADD_SIGNED:
      b->Write("gl_FragColor.rgb = arg0.rgb + arg1.rgb - kHalf.rgb;");
      break;
    case GL_INTERPOLATE:
      b->Write(
          "gl_FragColor.rgb = arg0.rgb * arg2.rgb + arg1.rgb * "
          "(kOne.rgb - arg2.rgb);");
      break;
    case GL_SUBTRACT:
      b->Write("gl_FragColor.rgb = arg0.rgb - arg1.rgb;");
      break;
    case GL_DOT3_RGB:
      b->Write(
          "gl_FragColor.rgb = 4.0 * dot(arg0.rgb - kHalf.rgb, "
          "arg1.rgb - kHalf.rgb) * kOne.rgb;");
      break;
    case GL_DOT3_RGBA:
      b->Write(
          "gl_FragColor = 4.0 * dot(arg0.rgb - kHalf.rgb, "
          "arg1.rgb - kHalf.rgb) * kOne;");
      break;
    default:
      LOG_ALWAYS_FATAL("Unknown operation %d (%s)", color_combine_op,
                       GetEnumString(color_combine_op));
  }

  // es_full_spec.1.1.12.pdf table 3.17.
  if (color_combine_op != GL_DOT3_RGBA) {
    switch (alpha_combine_op) {
      case GL_REPLACE:
        b->Write("gl_FragColor.a = arg0.a;");
        break;
      case GL_MODULATE:
        b->Write("gl_FragColor.a = arg0.a * arg1.a;");
        break;
      case GL_ADD:
        b->Write("gl_FragColor.a = arg0.a + arg1.a;");
        break;
      case GL_ADD_SIGNED:
        b->Write("gl_FragColor.a = arg0.a + arg1.a - 0.5;");
        break;
      case GL_INTERPOLATE:
        b->Write("gl_FragColor.a = arg0.a * arg2.a + arg1.a * (1.0 - arg2.a);");
        break;
      case GL_SUBTRACT:
        b->Write("gl_FragColor.a = arg0.a - arg1.a;");
        break;
      default:
        LOG_ALWAYS_FATAL("Unknown operation %d (%s)", alpha_combine_op,
                         GetEnumString(alpha_combine_op));
    }
  }

  // Apply the scale and clamping for this combine operation.
  b->Format("gl_FragColor *= u_texenv_scale[%d];", stage);
  b->Write("gl_FragColor = clamp(gl_FragColor, kZero, kOne);");
}

void WriteFSCompositeAllColors(const ShaderConfig& cfg, StringBuilder* b) {
  // es_full_spec.1.1.12.pdf section 3.7.12
  //
  // This function does all the work involved in compositing all the color
  // sources together (the color from the vertex shader, colors from any
  // texture bitmaps, from the texture environment, ...)
  //
  // This is the most complicated part of the GLES1 compatability shader due to
  // the sheer number of combinations of options. Each composite calculation is
  // relatively straightforward, there are just so many of them.

  // fragment always holds the fragment color prior to any composite operations.
  b->Write("mediump vec4 fragment = gl_FragColor;");
  // previous always holds the result of the last composite operation (or the
  // vertex color for the first stage.
  b->Write("mediump vec4 previous;");
  // sampler contains the color of the texture for each stage.
  b->Write("mediump vec4 sampler;");
  // constant contains the constant color for each stage.
  b->Write("mediump vec4 constant;");
  // These are used by the GL_COMBINE operation.
  b->Write("mediump vec4 arg0;");
  b->Write("mediump vec4 arg1;");
  b->Write("mediump vec4 arg2;");
  b->Write("mediump vec4 kZero = vec4(0.0, 0.0, 0.0, 0.0);");
  b->Write("mediump vec4 kHalf = vec4(0.5, 0.5, 0.5, 0.5);");
  b->Write("mediump vec4 kOne  = vec4(1.0, 1.0, 1.0, 1.0);");

  for (int stage = 0; stage < UniformContext::kMaxTextureUnits; ++stage) {
    const ShaderConfig::TextureConfig& tex = cfg.texture[stage];
    if (!tex.enabled) {
      continue;
    }

    b->Format("/* CompositeStage(%d) */", stage);
    b->Write("previous = gl_FragColor;");
    b->Format("constant = u_texenv_const[%d];", stage);
    if (tex.target == GL_TEXTURE_2D) {
      b->Format("sampler = texture2D(u_sampler_%d, v_texcoord[%d].xy);", stage,
                stage);
    } else if (tex.target == GL_TEXTURE_CUBE_MAP_OES) {
      b->Format("sampler = textureCube(u_sampler_%d, v_texcoord[%d].xyz);",
                stage, stage);
    } else {
      LOG_ALWAYS_FATAL("Unknown texture target %d (%s)", tex.target,
                       GetEnumString(tex.target));
    }

    // When reading texture data from memory and unpacking it into the RGBA form
    // used here, note these rules:
    //   * If the format does not have any color components (Alpha only), the
    //     RGB components will all end up being zero.
    //   * If the format does not have an alpha component, the .a component will
    //     end up being set to one.
    switch (tex.mode) {
      case GL_REPLACE:
        WriteFSCompositeReplace(tex.format, b);
        break;
      case GL_MODULATE:
        WriteFSCompositeModulate(tex.format, b);
        break;
      case GL_DECAL:
        WriteFSCompositeDecal(tex.format, b);
        break;
      case GL_BLEND:
        WriteFSCompositeBlend(tex.format, b);
        break;
      case GL_ADD:
        WriteFSCompositeAdd(tex.format, b);
        break;
      case GL_COMBINE:
        WriteFSCompositeCombine(tex, stage, b);
        break;
      default:
        LOG_ALWAYS_FATAL("Unknown texture environment mode %d (%s)",
                         tex.mode, GetEnumString(tex.mode));
    }
  }
}

void WriteFSComputeFog(const ShaderConfig& cfg, StringBuilder* b) {
  // es_full_spec.1.1.12.pdf section 3.8
  if (cfg.enable_fog) {
    switch (cfg.fog_mode) {
      case GL_LINEAR:
        b->Write(
            "mediump float fog = (u_fog_end - v_fog_distance) /"
            " (u_fog_end - u_fog_start);");
        break;
      case GL_EXP:
        b->Write("mediump float fog = exp(-(u_fog_density * v_fog_distance));");
        break;
      case GL_EXP2:
        b->Write(
            "mediump float fog = exp(-(u_fog_density * u_fog_density *"
            " v_fog_distance * v_fog_distance));");
        break;
      default:
        LOG_ALWAYS_FATAL("Unknown Fog Mode: %s", GetEnumString(cfg.fog_mode));
    }
    b->Write("fog = clamp(fog, 0.0, 1.0);");
    b->Write("gl_FragColor.rgb = mix(u_fog_color.rgb, gl_FragColor.rgb, fog);");
  }
}

void WriteFSTestFragmentAlpha(const ShaderConfig& cfg, StringBuilder* b) {
  // es_full_spec.1.1.12.pdf section 4.1.4
  // In GLES1, a test is performed in hardware on the final fragment alpha with
  // the intent of only rendering fragments that pass the test.
  if (!cfg.enable_alpha_test) {
    return;
  }

  switch (cfg.alpha_func) {
    case GL_ALWAYS:
      // Do nothing.
      break;
    case GL_NEVER:
      b->Write("discard;");
      break;
    case GL_EQUAL:
      b->Write("if (gl_FragColor.a != u_alpha_test_constant) { discard; }");
      break;
    case GL_NOTEQUAL:
      b->Write("if (gl_FragColor.a == u_alpha_test_constant) { discard; }");
      break;
    case GL_LESS:
      b->Write("if (gl_FragColor.a >= u_alpha_test_constant) { discard; }");
      break;
    case GL_LEQUAL:
      b->Write("if (gl_FragColor.a > u_alpha_test_constant) { discard; }");
      break;
    case GL_GREATER:
      b->Write("if (gl_FragColor.a <= u_alpha_test_constant) { discard; }");
      break;
    case GL_GEQUAL:
      b->Write("if (gl_FragColor.a < u_alpha_test_constant) { discard; }");
      break;
    default:
      LOG_ALWAYS_FATAL("Unknown Alpha Func: %s", GetEnumString(cfg.alpha_func));
  }
}

void WriteVSPositionAndNormal(const ShaderConfig& cfg, StringBuilder* b) {
  b->Write("vec4 eye_position;");
  if (NeedsNormal(cfg)) {
    b->Write("vec3 normal;");
  }

  if (cfg.enable_matrix_palette) {
    b->Format("for (int i = 0; i < %d; i++) {", cfg.vertex_units);
    b->Write("  int matrix_index = int(a_matrix_index[i]);");
    b->Write("  mat4 matrix = u_palette_matrix[matrix_index];");
    b->Write("  eye_position += a_weight[i] * matrix * a_position;");
    if (NeedsNormal(cfg)) {
      b->Write("  matrix = u_palette_inv_matrix[matrix_index];");
      b->Write("  normal += a_weight[i] * mat3(matrix) * a_normal;");
    }
    b->Write("}");
  } else {
    b->Write("eye_position = u_mv_matrix * a_position;");
    if (NeedsNormal(cfg)) {
      b->Write("normal = mat3(u_mv_inverse_transpose_matrix) * a_normal;");
    }
  }

  b->Write("gl_Position = u_p_matrix * eye_position;");
  if (cfg.enable_normalize && NeedsNormal(cfg)) {
    b->Write("normal = normalize(normal);");
  }
}

}  // namespace

void GenerateVertexShader(const ShaderConfig& config,
                          char* output_buffer, size_t output_buffer_size) {
  StringBuilder buffer(output_buffer, output_buffer_size);

  WriteVSHeader(config, &buffer);

  buffer.Write("void main() {");
  WriteVSPositionAndNormal(config, &buffer);
  WriteVSComputeClipPlaneDistances(config, &buffer);
  WriteVSComputeTextureCoordinates(config, &buffer);
  WriteVSComputeLighting(config, &buffer);
  WriteVSComputeFogDistance(config, &buffer);
  WriteVSComputePointSize(config, &buffer);
  buffer.Write("}");
}

void GenerateFragmentShader(const ShaderConfig& config,
                            char* output_buffer, size_t output_buffer_size) {
  StringBuilder buffer(output_buffer, output_buffer_size);

  WriteFSHeader(config, &buffer);

  buffer.Write("void main() {");
  if (config.mode == GL_POINTS) {
    WriteFSTestPointShape(config, &buffer);
  }
  WriteFSTestClipPlanes(config, &buffer);
  WriteFSSelectFaceColor(config, &buffer);
  WriteFSCompositeAllColors(config, &buffer);
  WriteFSComputeFog(config, &buffer);
  WriteFSTestFragmentAlpha(config, &buffer);
  buffer.Write("}");
}
