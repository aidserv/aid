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

#include "gles/state.h"

#include <algorithm>
#include <stdio.h>
#include <GLES/glext.h>

#include "common/alog.h"
#include "common/dlog.h"
#include "gles/gles_options.h"
#include "gles/buffer_data.h"
#include "gles/debug.h"
#include "gles/gles_context.h"
#include "gles/macros.h"

#define UNIFORM_KEY(enum, name) name,
const char* kUniformNames[] = {UNIFORM_KEY_TUPLE};
#undef UNIFORM_KEY

inline int Log2Floor(int max_texture_size) {
  const double size = static_cast<double>(max_texture_size);
  return floor(log(size) / log(2.0));
}

struct ElementRange {
  ElementRange() : first(0), last(0) {}
  size_t first;
  size_t last;
};

static ElementRange GetElementRange(GLsizei count, GLenum type,
                                    const GLvoid* data) {
  ElementRange range;
  if (count == 0) {
    return range;
  } else if (type == GL_UNSIGNED_BYTE) {
    const GLubyte* arr = (const GLubyte*)data;
    range.first = arr[0];
    range.last = arr[0];
    for (int i = 1; i < count; ++i) {
      range.first = std::min<size_t>(arr[i], range.first);
      range.last = std::max<size_t>(arr[i], range.last);
    }
  } else if (type == GL_UNSIGNED_SHORT) {
    const GLushort* arr = (const GLushort*)data;
    range.first = arr[0];
    range.last = arr[0];
    for (int i = 1; i < count; ++i) {
      range.first = std::min<size_t>(arr[i], range.first);
      range.last = std::max<size_t>(arr[i], range.last);
    }
  } else if (type == GL_UNSIGNED_INT) {
    const GLuint* arr = (const GLuint*)data;
    range.first = arr[0];
    range.last = arr[0];
    for (int i = 1; i < count; ++i) {
      range.first = std::min<size_t>(arr[i], range.first);
      range.last = std::max<size_t>(arr[i], range.last);
    }
  } else {
    LOG_ALWAYS_FATAL("Unknown type");
  }
  return range;
}

static GLint GetTypeSize(GLenum type) {
  switch (type) {
    case GL_BYTE:
      return sizeof(GLbyte);
    case GL_UNSIGNED_BYTE:
      return sizeof(GLubyte);
    case GL_SHORT:
      return sizeof(GLshort);
    case GL_UNSIGNED_SHORT:
      return sizeof(GLushort);
    case GL_INT:
      return sizeof(GLint);
    case GL_UNSIGNED_INT:
      return sizeof(GLuint);
    case GL_FLOAT:
      return sizeof(GLfloat);
    case GL_FIXED:
      return sizeof(GLfixed);
    default:
      LOG_ALWAYS_FATAL("Unknown GL type: %d", type);
      return -1;
  }
}

// Default values for lights can be found here:
// es_full_spec.1.1.12.pdf section 2.12.1 table 2.8.
Light::Light()
    : ambient(0.f, 0.f, 0.f, 1.f),
      diffuse(0.f, 0.f, 0.f, 1.f),
      specular(0.f, 0.f, 0.f, 1.f),
      position(0.f, 0.f, 1.f, 0.f),
      direction(0.f, 0.f, -1.f, 0.f),
      spot_exponent(0.f),
      spot_cutoff(180.f),
      const_attenuation(1.f),
      linear_attenuation(0.f),
      quadratic_attenuation(0.f) {}

bool Light::IsSpot() const { return spot_cutoff != 180.f; }

bool Light::IsPositional() const { return position.Get(3) != 0.f; }

bool Light::IsDirectional() const { return position.Get(3) == 0.f; }

bool Light::ShouldAttenuate() const {
  return IsPositional() &&
         (const_attenuation != 1.f || linear_attenuation != 0.f ||
          quadratic_attenuation != 0.f);
}

// Default values for lights can be found here:
// es_full_spec.1.1.12.pdf section 2.12.1 table 2.8.
Material::Material()
    : ambient(0.2f, 0.2f, 0.2f, 1.0f),
      diffuse(0.8f, 0.8f, 0.8f, 1.0f),
      specular(0.f, 0.f, 0.f, 1.f),
      emission(0.f, 0.f, 0.f, 1.f),
      shininess(0.f) {}

// Default values for lights can be found here:
// es_full_spec.1.1.12.pdf section 3.8.
Fog::Fog()
    : mode(GL_EXP),
      color(0.f, 0.f, 0.f, 0.f),
      density(1.f),
      start(0.f),
      end(1.f) {}

// See es_full_spec.1.1.12.pdf section 3.7.12.
// The initial values are defined at the end of the section.
TexEnv::TexEnv()
    : mode(GL_MODULATE),
      combine_rgb(GL_MODULATE),
      combine_alpha(GL_MODULATE),
      color(0.f, 0.f, 0.f, 0.f),
      rgb_scale(1.0f),
      alpha_scale(1.0f) {
  src_rgb[0] = GL_TEXTURE;
  src_rgb[1] = GL_PREVIOUS;
  src_rgb[2] = GL_CONSTANT;
  src_alpha[0] = GL_TEXTURE;
  src_alpha[1] = GL_PREVIOUS;
  src_alpha[2] = GL_CONSTANT;
  operand_rgb[0] = GL_SRC_COLOR;
  operand_rgb[1] = GL_SRC_COLOR;
  operand_rgb[2] = GL_SRC_ALPHA;  // Note: alpha here conforms to spec
  operand_alpha[0] = GL_SRC_ALPHA;
  operand_alpha[1] = GL_SRC_ALPHA;
  operand_alpha[2] = GL_SRC_ALPHA;
}

// See https://www.khronos.org/registry/gles/extensions/OES/OES_texture_cube_map.txt
// The initial values are defined in Section 2.11.4
TexGen::TexGen()
    : mode(GL_REFLECTION_MAP_OES),
      enabled(GL_FALSE) {
}

// See es_full_spec.1.1.12.pdf section 6.2
// The initial values are defined in table 6.11
PointParameters::PointParameters()
    : size_min(0.f),
      size_max(1.f),
      current_size(1.f) {
  attenuation[0] = 1.f;
  attenuation[1] = 0.f;
  attenuation[2] = 0.f;
}

AlphaTest::AlphaTest()
    : func(GL_ALWAYS), value(0.f) {
}

PointerData::PointerData()
  : enabled(false),
    size(4),
    type(GL_FLOAT),
    stride(0),
    normalize(GL_FALSE),
    buffer_name(0),
    pointer(NULL) {
}

void StateCacheLoaderHelper::Load(GLenum value, GLboolean* data) {
  GlesContext* c = GetCurrentGlesContext();
  LOG_ALWAYS_FATAL_IF(c == NULL);
  PASS_THROUGH(c, GetBooleanv, value, data);
}

void StateCacheLoaderHelper::Load(GLenum value, GLfloat* data) {
  GlesContext* c = GetCurrentGlesContext();
  LOG_ALWAYS_FATAL_IF(c == NULL);
  PASS_THROUGH(c, GetFloatv, value, data);
}

void StateCacheLoaderHelper::Load(GLenum value, GLint* data) {
  GlesContext* c = GetCurrentGlesContext();
  LOG_ALWAYS_FATAL_IF(c == NULL);
  PASS_THROUGH(c, GetIntegerv, value, data);
}

void StateCacheLoaderHelper::Load(GLenum value, GLuint* data) {
  GlesContext* c = GetCurrentGlesContext();
  LOG_ALWAYS_FATAL_IF(c == NULL);
  PASS_THROUGH(c, GetIntegerv, value, reinterpret_cast<GLint*>(data));
}

void StateCacheLoaderHelper::Load(
    GLenum value, GLboolean& data) {  // NOLINT(runtime/references)
  Load(value, &data);
}

void StateCacheLoaderHelper::Load(
    GLenum value, GLfloat& data) {  // NOLINT(runtime/references)
  Load(value, &data);
}

void StateCacheLoaderHelper::Load(
    GLenum value, GLint& data) {  // NOLINT(runtime/references)
  Load(value, &data);
}

void StateCacheLoaderHelper::Load(
    GLenum value, GLuint& data) {  // NOLINT(runtime/references)
  Load(value, &data);
}

namespace {

const GLfloat kFullscreenQuadVertices[] = {
  // position
  -1.0f, -1.0f,
  +1.0f, -1.0f,
  -1.0f, +1.0f,
  +1.0f, +1.0f,

  // texcoord
  0.0f, 0.0f,
  1.0f, 0.0f,
  0.0f, 1.0f,
  1.0f, 1.0f,

  // flipped texcoord
  0.0f, 1.0f,
  1.0f, 1.0f,
  0.0f, 0.0f,
  1.0f, 0.0f,
};

const size_t kNormalUVOffset = sizeof(GLfloat) * 8;
const size_t kFlippedUVOffset = sizeof(GLfloat) * 16;

const char kVertexShader[] =
  "attribute mediump vec2 a_position;"
  "attribute mediump vec2 a_texcoord;"
  "varying mediump vec2 v_texcoord;"
  "void main() {"
  "  v_texcoord = a_texcoord;"
  "  gl_Position = vec4(a_position.x, a_position.y, 0.0, 1.0);"
  "}";

const char kFragmentShader[] =
  "uniform sampler2D u_sampler;"
  "varying mediump vec2 v_texcoord;"
  "void main() {"
  "  gl_FragColor = texture2D(u_sampler, v_texcoord);"
  "}";

}  // namespace

FullscreenQuad::FullscreenQuad(GlesContext* context)
  : context_(context),
    program_(0),
    buffer_(0),
    position_attrib_idx_(-1),
    sampler_uniform_idx_(-1) {

  const GLuint vsh = context_->CompileShader(GL_VERTEX_SHADER, kVertexShader);
  const GLuint fsh = context_->CompileShader(GL_FRAGMENT_SHADER,
                                             kFragmentShader);

  program_ = context_->CompileProgram(vsh, fsh);
  position_attrib_idx_ = PASS_THROUGH(context_, GetAttribLocation, program_,
                                      "a_position");
  texcoord_attrib_idx_ = PASS_THROUGH(context_, GetAttribLocation, program_,
                                      "a_texcoord");
  sampler_uniform_idx_ = PASS_THROUGH(context_, GetUniformLocation, program_,
                                      "u_sampler");
  PASS_THROUGH(context_, DeleteShader, vsh);
  PASS_THROUGH(context_, DeleteShader, fsh);
  PASS_THROUGH(context_, GenBuffers, 1, &buffer_);
  PASS_THROUGH(context_, BindBuffer, GL_ARRAY_BUFFER, buffer_);
  PASS_THROUGH(context_, BufferData, GL_ARRAY_BUFFER,
               sizeof(kFullscreenQuadVertices), kFullscreenQuadVertices,
               GL_STATIC_DRAW);
  PASS_THROUGH(context_, UseProgram, program_);
  PASS_THROUGH(context_, Uniform1i, sampler_uniform_idx_, 0);
}

FullscreenQuad::~FullscreenQuad() {
  PASS_THROUGH(context_, DeleteBuffers, 1, &buffer_);
  PASS_THROUGH(context_, DeleteProgram, program_);
}

void FullscreenQuad::Draw(GLuint texture, bool flip_v) {
  const GLuint global_name =
    context_->GetShareGroup()->GetTextureGlobalName(texture);
  LOG_ALWAYS_FATAL_IF(global_name == 0);
  PASS_THROUGH(context_, UseProgram, program_);
  PASS_THROUGH(context_, ActiveTexture, GL_TEXTURE0);
  PASS_THROUGH(context_, BindTexture, GL_TEXTURE_2D, global_name);
  PASS_THROUGH(context_, EnableVertexAttribArray, position_attrib_idx_);
  PASS_THROUGH(context_, EnableVertexAttribArray, texcoord_attrib_idx_);
  PASS_THROUGH(context_, BindBuffer, GL_ARRAY_BUFFER, buffer_);
  PASS_THROUGH(context_, VertexAttribPointer, position_attrib_idx_, 2,
               GL_FLOAT, GL_FALSE, 0, 0);
  const size_t offset = flip_v ? kFlippedUVOffset : kNormalUVOffset;
  PASS_THROUGH(context_, VertexAttribPointer, texcoord_attrib_idx_, 2,
               GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(offset));
  PASS_THROUGH(context_, DrawArrays, GL_TRIANGLE_STRIP, 0, 4);
  PASS_THROUGH(context_, DisableVertexAttribArray, position_attrib_idx_);
  PASS_THROUGH(context_, DisableVertexAttribArray, texcoord_attrib_idx_);
}

// Default values can be found here:
// color:es_full_spec.1.1.12.pdf section 2.7
// normal: es_full_spec.1.1.12.pdf section 2.7
// ambient: es_full_spec.1.1.12.pdf section 2.12.1 table 2.8.
UniformContext::UniformContext(GlesContext* context)
    : context_(context),
      program_object_(0),
      active_texture_unit_(0),
      current_palette_matrix_(0),
      ambient_(emugl::Vector(0.2f, 0.2f, 0.2f, 1.f)),
      color_(emugl::Vector(1.f, 1.f, 1.f, 1.f)),
      normal_(emugl::Vector(0.f, 0.f, 1.f, 0.f)) {
  DLOG("Created uniform context for GLES1 context @ %p", context);

  // GL_MODELVIEW is the default matrix mode.
  // es_full_spec.1.1.12.pdf section 2.10.2.
  SetMatrixMode(GL_MODELVIEW);

  // Some default values for LIGHT0 are different than the other lights.
  // es_full_spec.1.1.12.pdf section 2.12.1 table 2.8.
  lights_[0].Mutate().diffuse = emugl::Vector(1.f, 1.f, 1.f, 1.f);
  lights_[0].Mutate().specular = emugl::Vector(1.f, 1.f, 1.f, 1.f);
}

void UniformContext::Init(int num_texture_units) {
  DLOG("Initialized uniform context for GLES1 context @ %p", context_);
  LOG_ALWAYS_FATAL_IF(num_texture_units != kMaxTextureUnits);
}

void UniformContext::Bind(ProgramContext* program) {
  LOG_ALWAYS_FATAL_IF(!program);

  // If the program has changed, we need to make sure all the uniforms
  // are updated as the new program may have restored out-of-date uniform
  // data.
  const bool force_update = (program_object_ != program->GetProgramObject());
  const bool mv_is_dirty = modelview_matrix_stack_.IsDirty();
  const bool p_is_dirty = projection_matrix_stack_.IsDirty();
  if (force_update || mv_is_dirty) {
    const emugl::Matrix& model_view_matrix =
        modelview_matrix_stack_.Get().GetTop();
    // TODO(crbug.com/441941): Use the adjoint of the model view matrix for
    // computing normals.
    if (program->GetUniformLocation(
        kModelViewInverseTransposeMatrixUniform) != -1) {
      emugl::Matrix mv_inverse_transpose = model_view_matrix;
      mv_inverse_transpose.Inverse();
      mv_inverse_transpose.Transpose();
      if (context_->IsEnabled(GL_RESCALE_NORMAL)) {
        mv_inverse_transpose.RescaleNormal();
      }
      program->BindUniform(mv_inverse_transpose,
                           kModelViewInverseTransposeMatrixUniform);
    }
    program->BindUniform(model_view_matrix, kModelViewMatrixUniform);
    modelview_matrix_stack_.Clean();
  }

  if (force_update || p_is_dirty) {
    const emugl::Matrix& projection_matrix =
        projection_matrix_stack_.Get().GetTop();
    program->BindUniform(projection_matrix, kProjectionMatrixUniform);
    projection_matrix_stack_.Clean();
  }

  if (force_update || alpha_test_.IsDirty()) {
    program->BindUniform(alpha_test_.Get().value, kAlphaTestConstantUniform);
    alpha_test_.Clean();
  }
  if (force_update || ambient_.IsDirty()) {
    program->BindUniform(ambient_.Get(), kAmbientLightUniform);
    ambient_.Clean();
  }
  if (force_update || material_.IsDirty()) {
    program->BindUniform(material_.Get());
    material_.Clean();
  }
  if (force_update || fog_.IsDirty()) {
    program->BindUniform(fog_.Get());
    fog_.Clean();
  }
  if (force_update || point_parameters_.IsDirty()) {
    program->BindUniform(point_parameters_.Get());
    point_parameters_.Clean();
  }
  // TODO(crbug.com/441942): Bind light uniforms as one array of data rather
  // than individually with a loop.
  for (int i = 0; i < kMaxLights; ++i) {
    if (force_update || lights_[i].IsDirty()) {
      program->BindUniform(lights_[i].Get(), i);
      lights_[i].Clean();
    }
  }
  // TODO(crbug.com/441942): Bind clip plane uniforms as one array of data
  // rather than individually with a loop.
  for (int i = 0; i < kMaxClipPlanes; ++i) {
    if (force_update || clip_planes_[i].IsDirty()) {
      program->BindUniform(clip_planes_[i].Get(), kClipPlaneUniform, i);
      clip_planes_[i].Clean();
    }
  }
  // TODO(crbug.com/441942): Bind texture uniforms as one array of data rather
  // than individually with a loop.
  for (int i = 0; i < kMaxTextureUnits; ++i) {
    // Bind sampler N to texture unit N.
    const GLint sampler_id = i;
    program->BindUniform(sampler_id, kTextureSamplerUniform, i);

    if (force_update || texenv_[i].IsDirty()) {
      program->BindUniform(texenv_[i].Get(), i);
      texenv_[i].Clean();
    }
    if (force_update || texture_matrix_stack_[i].IsDirty()) {
      const emugl::Matrix& texture_matrix =
          texture_matrix_stack_[i].Get().GetTop();
      program->BindUniform(texture_matrix, kTextureMatrixUniform, i);
      texture_matrix_stack_[i].Clean();
    }
  }

  for (int i = 0; i < kMaxPaletteMatricesOES; ++i) {
    if (force_update || palette_matrices_[i].IsDirty()) {
      emugl::Matrix matrix = palette_matrices_[i].Get();
      program->BindUniform(matrix, kPaletteMatrixUniform, i);
      matrix.Inverse();
      matrix.Transpose();
      program->BindUniform(matrix, kPaletteInverseMatrixUniform, i);
      palette_matrices_[i].Clean();
    }
  }

  program_object_  = program->GetProgramObject();
}

void UniformContext::Enable(GLenum cap) {
  if (cap == GL_TEXTURE_GEN_STR_OES) {
    MutateActiveTexGen().enabled = GL_TRUE;
  }
}

void UniformContext::Disable(GLenum cap) {
  if (cap == GL_TEXTURE_GEN_STR_OES) {
    MutateActiveTexGen().enabled = GL_FALSE;
  }
}

GLboolean UniformContext::IsEnabled(GLenum cap) const {
  if (cap == GL_TEXTURE_GEN_STR_OES) {
    return GetActiveTexGen().enabled;
  }
  return GL_FALSE;
}

bool UniformContext::SetMatrixMode(GLenum mode) {
  switch (mode) {
    case GL_PROJECTION:
    case GL_MATRIX_PALETTE_OES:
    case GL_MODELVIEW:
    case GL_TEXTURE:
      matrix_mode_ = mode;
      return true;
    default:
      return false;
  }
}

bool UniformContext::SetActiveTexture(GLenum texture_unit) {
  const int stage = texture_unit - GL_TEXTURE0;
  if (stage >= 0 && stage < kMaxTextureUnits) {
    active_texture_unit_ = stage;
    return true;
  }
  return false;
}

const emugl::Matrix& UniformContext::GetModelViewMatrix() const {
  return modelview_matrix_stack_.Get().GetTop();
}

emugl::Matrix& UniformContext::MutateModelViewMatrix() {
  return modelview_matrix_stack_.Mutate().GetTop();
}

const emugl::Matrix& UniformContext::GetProjectionMatrix() const {
  return projection_matrix_stack_.Get().GetTop();
}

emugl::Matrix& UniformContext::MutateProjectionMatrix() {
  return projection_matrix_stack_.Mutate().GetTop();
}

const emugl::Matrix& UniformContext::GetTextureMatrix() const {
  return texture_matrix_stack_[active_texture_unit_].Get().GetTop();
}

const emugl::Matrix& UniformContext::GetTextureMatrixByStage(GLint stage) const {
  LOG_ALWAYS_FATAL_IF(stage < 0 || stage >= kMaxTextureUnits);
  return texture_matrix_stack_[stage].Get().GetTop();
}

emugl::Matrix& UniformContext::MutateTextureMatrixByStage(GLint stage) {
  LOG_ALWAYS_FATAL_IF(stage < 0 || stage >= kMaxTextureUnits);
  return texture_matrix_stack_[stage].Mutate().GetTop();
}

bool UniformContext::ActiveMatrixPush() {
  if (matrix_mode_ == GL_MATRIX_PALETTE_OES) {
    return false;
  }
  const Dirtiable<MatrixStack>* stack = GetActiveMatrixStackInternal();
  return const_cast<Dirtiable<MatrixStack>*>(stack)->Mutate().Push();
}

bool UniformContext::ActiveMatrixPop() {
  if (matrix_mode_ == GL_MATRIX_PALETTE_OES) {
    return false;
  }
  const Dirtiable<MatrixStack>* stack = GetActiveMatrixStackInternal();
  return const_cast<Dirtiable<MatrixStack>*>(stack)->Mutate().Pop();
}

emugl::Matrix& UniformContext::MutateActiveMatrix() {
  if (matrix_mode_ == GL_MATRIX_PALETTE_OES) {
    return MutatePaletteMatrix();
  }
  const Dirtiable<MatrixStack>* stack = GetActiveMatrixStackInternal();
  return const_cast<Dirtiable<MatrixStack>*>(stack)->Mutate().GetTop();
}

const emugl::Matrix& UniformContext::GetActiveMatrix() const {
  if (matrix_mode_ == GL_MATRIX_PALETTE_OES) {
    return GetPaletteMatrix();
  }
  const Dirtiable<MatrixStack>* stack = GetActiveMatrixStackInternal();
  return stack->Get().GetTop();
}

bool UniformContext::SetCurrentPaletteMatrix(GLuint index) {
  if (index >= kMaxPaletteMatricesOES) {
    return false;
  }
  current_palette_matrix_ = index;
  return true;
}

const emugl::Matrix& UniformContext::GetPaletteMatrix() const {
  return palette_matrices_[current_palette_matrix_].Get();
}

emugl::Matrix& UniformContext::MutatePaletteMatrix() {
  return palette_matrices_[current_palette_matrix_].Mutate();
}

TexEnv& UniformContext::MutateActiveTexEnv() {
  return texenv_[active_texture_unit_].Mutate();
}

const TexEnv& UniformContext::GetActiveTexEnv() const {
  return texenv_[active_texture_unit_].Get();
}

TexGen& UniformContext::MutateActiveTexGen() {
  return texgen_[active_texture_unit_].Mutate();
}

const TexGen& UniformContext::GetActiveTexGen() const {
  return texgen_[active_texture_unit_].Get();
}

emugl::Vector* UniformContext::MutateClipPlane(GLenum id) {
  const int index = id - GL_CLIP_PLANE0;
  if (index >= 0 && index < kMaxClipPlanes) {
    return &clip_planes_[index].Mutate();
  }
  return NULL;
}

const emugl::Vector* UniformContext::GetClipPlane(GLenum id) const {
  const int index = id - GL_CLIP_PLANE0;
  if (index >= 0 && index < kMaxClipPlanes) {
    return &clip_planes_[index].Get();
  }
  return NULL;
}

TexEnv* UniformContext::MutateTexEnv(GLenum id) {
  const int stage = id - GL_TEXTURE0;
  if (stage >= 0 && stage < kMaxTextureUnits) {
    return &texenv_[stage].Mutate();
  }
  return NULL;
}

const TexEnv* UniformContext::GetTexEnv(GLenum id) const {
  const int stage = id - GL_TEXTURE0;
  if (stage >= 0 && stage < kMaxTextureUnits) {
    return &texenv_[stage].Get();
  }
  return NULL;
}

TexGen* UniformContext::MutateTexGen(GLenum id) {
  const int stage = id - GL_TEXTURE0;
  if (stage >= 0 && stage < kMaxTextureUnits) {
    return &texgen_[stage].Mutate();
  }
  return NULL;
}

const TexGen* UniformContext::GetTexGen(GLenum id) const {
  const int stage = id - GL_TEXTURE0;
  if (stage >= 0 && stage < kMaxTextureUnits) {
    return &texgen_[stage].Get();
  }
  return NULL;
}

Light* UniformContext::MutateLight(GLenum id) {
  const int index = id - GL_LIGHT0;
  if (index >= 0 && index < kMaxLights) {
    return &lights_[index].Mutate();
  }
  return NULL;
}

const Light* UniformContext::GetLight(GLenum id) const {
  const int index = id - GL_LIGHT0;
  if (index >= 0 && index < kMaxLights) {
    return &lights_[index].Get();
  }
  return NULL;
}

const Dirtiable<MatrixStack>*
UniformContext::GetActiveMatrixStackInternal() const {
  switch (matrix_mode_) {
    case GL_PROJECTION:
      return &projection_matrix_stack_;
    case GL_MODELVIEW:
      return &modelview_matrix_stack_;
    case GL_TEXTURE:
      return &texture_matrix_stack_[active_texture_unit_];
    default:
      return NULL;
  }
}

ProgramContext::ProgramContext(GlesContext* context, GLuint program)
    : context_(context), program_object_(program) {
        DLOG("Created program context for GLES1 context @ %p", context_);
    }

void ProgramContext::Bind() {
  PASS_THROUGH(context_, UseProgram, program_object_);
}

void ProgramContext::Delete() {
  PASS_THROUGH(context_, DeleteProgram, program_object_);
}

GLint ProgramContext::GetUniformLocation(UniformKey key) {
  return GetUniformLocation(key, kScalarUniform);
}

GLint ProgramContext::GetUniformLocation(UniformKey key, int idx) {
  const std::pair<UniformKey, int> entry = std::make_pair(key, idx);
  UniformLocationMap::iterator iter = uniform_location_map_.find(entry);
  if (iter != uniform_location_map_.end()) {
    return iter->second;
  }

  static const int kMaxBufferLength = 256;
  char buffer[kMaxBufferLength];

  const char* name = kUniformNames[key];
  if (strstr(name, "%d")) {
    LOG_ALWAYS_FATAL_IF(idx < 0, "%s is an array", name);
    snprintf(buffer, sizeof(buffer), name, idx);
  } else {
    LOG_ALWAYS_FATAL_IF(idx >= 0, "%s is not an array", name);
    strncpy(buffer, name, sizeof(buffer));
  }
  buffer[kMaxBufferLength - 1] = 0;

  const GLint location =
      PASS_THROUGH(context_, GetUniformLocation, program_object_, buffer);
  uniform_location_map_[entry] = location;
  return location;
}

void ProgramContext::BindUniform(const int& value, UniformKey key) {
  BindUniform(value, key, kScalarUniform);
}

void ProgramContext::BindUniform(const int& value, UniformKey key, int index) {
  const GLint location = GetUniformLocation(key, index);
  if (location != -1) {
    PASS_THROUGH(context_, Uniform1i, location, value);
  }
}

void ProgramContext::BindUniform(const float& value, UniformKey key) {
  BindUniform(value, key, kScalarUniform);
}

void ProgramContext::BindUniform(const float& value, UniformKey key,
                                 int index) {
  const GLint location = GetUniformLocation(key, index);
  if (location != -1) {
    PASS_THROUGH(context_, Uniform1f, location, value);
  }
}

void ProgramContext::BindUniform(const GLfloat (&values)[3], UniformKey key) {
  const int location = GetUniformLocation(key);
  if (location != -1) {
    PASS_THROUGH(context_, Uniform3fv, location, 1, values);
  }
}

void ProgramContext::BindUniform(const emugl::Vector& vector, UniformKey key) {
  BindUniform(vector, key, kScalarUniform);
}

void ProgramContext::BindUniform(const emugl::Vector& vector, UniformKey key,
                                 int index) {
  const GLint location = GetUniformLocation(key, index);
  if (location != -1) {
    GLfloat float_array[emugl::Vector::kEntries];
    vector.GetFloatArray(float_array);
    PASS_THROUGH(context_, Uniform4fv, location, 1, float_array);
  }
}

void ProgramContext::BindUniform(const emugl::Matrix& matrix, UniformKey key) {
  BindUniform(matrix, key, kScalarUniform);
}

void ProgramContext::BindUniform(const emugl::Matrix& matrix, UniformKey key,
                                 int index) {
  const GLint location = GetUniformLocation(key, index);
  if (location != -1) {
    GLfloat float_array[emugl::Matrix::kEntries];
    matrix.GetColumnMajorArray(float_array);
    PASS_THROUGH(context_, UniformMatrix4fv, location, 1, GL_FALSE,
                 float_array);
  }
}

void ProgramContext::BindUniform(const Fog& fog) {
  BindUniform(fog.density, kFogDensityUniform);
  BindUniform(fog.start, kFogStartUniform);
  BindUniform(fog.end, kFogEndUniform);
  BindUniform(fog.color, kFogColorUniform);
}

void ProgramContext::BindUniform(const Light& light, int index) {
  const float light_spot_cutoff =
      cos(light.spot_cutoff * emugl::kRadiansPerDegree);
  emugl::Vector light_position = light.position;
  if (light_position.Get(3) == 0.f) {
    light_position.Normalize();
  }
  emugl::Vector light_direction = light.direction;
  light_direction.Normalize();
  light_direction.Scale(-1.f);

  BindUniform(light.ambient, kLightAmbientUniform, index);
  BindUniform(light.diffuse, kLightDiffuseUniform, index);
  BindUniform(light.specular, kLightSpecularUniform, index);
  BindUniform(light_position, kLightPositionUniform, index);
  BindUniform(light_direction, kLightDirectionUniform, index);
  BindUniform(light.spot_exponent, kLightSpotExponentUniform, index);
  BindUniform(light_spot_cutoff, kLightSpotCutoffUniform, index);
  BindUniform(light.const_attenuation, kLightConstAttenuationUniform, index);
  BindUniform(light.linear_attenuation, kLightLinearAttenuationUniform, index);
  BindUniform(light.quadratic_attenuation, kLightQuadraticAttenuationUniform,
              index);
}

void ProgramContext::BindUniform(const Material& material) {
  BindUniform(material.ambient, kMaterialAmbientUniform);
  BindUniform(material.diffuse, kMaterialDiffuseUniform);
  BindUniform(material.specular, kMaterialSpecularUniform);
  BindUniform(material.emission, kMaterialEmissionUniform);
  BindUniform(material.shininess, kMaterialShininessUniform);
}

void ProgramContext::BindUniform(const PointParameters& point_parameters) {
  BindUniform(point_parameters.size_min, kPointSizeMinUniform);
  BindUniform(point_parameters.size_max, kPointSizeMaxUniform);
  BindUniform(point_parameters.attenuation, kPointSizeAttenuationUniform);
}

void ProgramContext::BindUniform(const TexEnv& texenv, int index) {
  // es_full_spec.1.1.12.pdf section 3.7.13
  // Load up the various constants used by the texture environment for each
  // supported stage.
  // Each stage has:
  //   * An constant color (RGBA), which can be used as a source.
  //   * A color/alpha scale. For simplicity we generate a four component scale
  //     here, though we only really need two components.
  BindUniform(texenv.color, kTextureEnvColorUniform, index);
  const emugl::Vector scale = emugl::Vector(texenv.rgb_scale, texenv.rgb_scale,
                              texenv.rgb_scale, texenv.alpha_scale);
  BindUniform(scale, kTextureEnvScaleUniform, index);
}

const GLenum TextureContext::supported_texture_targets_[kNumTargets] = {
    GL_TEXTURE_2D,
    GL_TEXTURE_CUBE_MAP,
    GL_TEXTURE_EXTERNAL_OES,
};

TextureContext::TextureContext(GlesContext* context)
  : context_(context),
    max_texture_levels_(0),
    num_texture_units_(0),
    active_texture_unit_(0),
    client_active_texture_unit_(0) {
        DLOG("Created texture context for GLES1 context @ %p", context_);
}

TextureContext::~TextureContext() {
}

void TextureContext::Init(int max_texture_units, int max_texture_size) {
        DLOG("Initialized texture context for GLES1 context @ %p", context_);
  ALOG_ASSERT(max_texture_size > 0);
  ALOG_ASSERT((max_texture_size & (max_texture_size - 1)) == 0);
  num_texture_units_ = max_texture_units;
  max_texture_levels_ = Log2Floor(max_texture_size);

  for (int i = 0; i < kNumTargets; ++i) {
    const GLenum target = supported_texture_targets_[i];
    texture_units_[i].resize(num_texture_units_);
    for (GLuint j = 0; j < num_texture_units_; ++j) {
      texture_units_[i][j].global_target = target;
    }

    TextureDataPtr data(new TextureData(0));
    data->Bind(target, max_texture_levels_);
    default_textures_.push_back(data);
  }
}

void TextureContext::DeleteTexture(GLuint texture) {
  for (int i = 0; i < kNumTargets; ++i) {
    for (GLuint j = 0; j < num_texture_units_; ++j) {
      TextureUnit& unit = texture_units_[i][j];
      if (unit.texture == texture) {
        unit.texture = 0;
      }
    }
  }
}

bool TextureContext::SetActiveTexture(GLenum texture_unit) {
  const GLuint stage = texture_unit - GL_TEXTURE0;
  if (stage < num_texture_units_) {
    active_texture_unit_ = stage;
    return true;
  }
  return false;
}

TextureDataPtr TextureContext::GetDefaultTextureData(GLenum target) {
  const Target t = MapTarget(target);
  return default_textures_[t];
}

bool TextureContext::BindTextureToTarget(const TextureDataPtr& tex,
                                         GLenum target) {
  const GLenum tex_target = tex->GetTarget();
  if (tex_target != 0) {
    if (MapTarget(tex_target) != MapTarget(target)) {
      ALOGW("Cannot change texture target once bound.");
      return false;
    }
  } else {
    tex->Bind(target, max_texture_levels_);
  }
  return true;
}

void TextureContext::SetTargetTexture(GLenum target, GLuint texture,
                                      GLenum global_target) {
  // The logic below in PrepareTextures/RestoreTextures assumes that we are only
  // ever remapping TEXTURE_EXTERNAL_OES to TEXTURE_2D.
  LOG_ALWAYS_FATAL_IF(target != global_target &&
                      (global_target != GL_TEXTURE_2D ||
                       target != GL_TEXTURE_EXTERNAL_OES),
                      "Unsupported mapping of target(%d) and "
                      "global_target(%d)", target, global_target);

  const Target t = MapTarget(target);
  TextureUnit& unit = GetTextureUnit(t, active_texture_unit_);
  unit.texture = texture;
  unit.global_target = global_target;
}

GLenum TextureContext::EnsureCorrectBinding(GLenum target) {
  const TextureUnit& unit = GetTextureUnit(MapTarget(target),
                                           active_texture_unit_);
  // We only need to rebind if there is a remapping in effect.
  if (target != unit.global_target) {
    BindUnderlying(unit);
  }
  return unit.global_target;
}

void TextureContext::RestoreBinding(GLenum target) {
  const TextureUnit& unit = GetTextureUnit(MapTarget(target),
                                           active_texture_unit_);
  // We only need to restore if there is a remapping in effect.
  if (unit.global_target != target) {
    const TextureUnit& orig = GetTextureUnit(MapTarget(unit.global_target),
                                             active_texture_unit_);
    // We only rebind if that texture is expected at that local target
    if (orig.global_target == target) {
      BindUnderlying(orig);
    }
  }
}

void TextureContext::PrepareTextures(bool gles11,
                                     bool shader_uses_external_as_2d) {
  GLuint current_active_texture_unit = active_texture_unit_;
  for (GLuint i = 0; i < num_texture_units_; ++i) {
    const TextureUnit& unit_ext = GetTextureUnit(kTextureExternal, i);

    // We only need to do worry about rebinding textures if we have a
    // external texture that is actually supposed to be a TEXTURE_2D.
    if (unit_ext.global_target == GL_TEXTURE_2D) {
      // For the GLES1 code path, we know if the texture target was enabled or
      // not. If it is enabled, the external texture overrides any normal
      // texture 2d according to the specification.
      // For the GLES2+ code path, things are much more interesting, and
      // we need to know what uniform type the shader uses for each texture
      // unit, as valid textures can be bound to each target and the shader
      // defines which one is read.
      // TODO(crbug.com/441940): This really needs to be revisited to not
      // assume that only the first texture unit is the one that needs to be
      // remapped.
      bool bind_ext_as_2d = ((gles11 && unit_ext.enabled) ||
                             (i == 0 && shader_uses_external_as_2d));

      ActiveTextureUnderlying(i);
      current_active_texture_unit = i;
      const TextureUnit& unit_2d = GetTextureUnit(kTexture2d, i);
      // If the app was using the unit associated with external textures
      // bind that unit's data, otherwise the app was using 2D-related unit.
      BindUnderlying(bind_ext_as_2d ? unit_ext : unit_2d);
    }
  }
  if (current_active_texture_unit != active_texture_unit_) {
    ActiveTextureUnderlying(active_texture_unit_);
  }
}

void TextureContext::RestoreTextures() {
  GLuint current_active_texture_unit = active_texture_unit_;
  for (GLuint i = 0; i < num_texture_units_; ++i) {
    const TextureUnit& unit_ext = GetTextureUnit(kTextureExternal, i);
    // If we have an external texture that maps to TEXTURE_2D we simply rebind
    // the 2D texture unconditionally rather than duplicate the logic from
    // PrepareTextures. Worst case this texture is already bound.
    if (unit_ext.global_target == GL_TEXTURE_2D) {
      ActiveTextureUnderlying(i);
      current_active_texture_unit = i;
      BindUnderlying(GetTextureUnit(kTexture2d, i));
    }
  }
  if (current_active_texture_unit != active_texture_unit_) {
    ActiveTextureUnderlying(active_texture_unit_);
  }
}

void TextureContext::Enable(GLenum target) {
  const Target t = MapTarget(target);
  TextureUnit& unit = GetTextureUnit(t, active_texture_unit_);
  unit.enabled = true;
}

void TextureContext::Disable(GLenum target) {
  const Target t = MapTarget(target);
  TextureUnit& unit = GetTextureUnit(t, active_texture_unit_);
  unit.enabled = false;
}

bool TextureContext::IsEnabled(GLenum target) const {
  return IsEnabled(active_texture_unit_ + GL_TEXTURE0, target);
}

bool TextureContext::IsEnabled(GLenum id, GLenum target) const {
  const GLuint index = id - GL_TEXTURE0;
  LOG_ALWAYS_FATAL_IF(index >= num_texture_units_);
  const Target t = MapTarget(target);
  const TextureUnit& unit = GetTextureUnit(t, index);
  return unit.enabled;
}

GLuint TextureContext::GetBoundTexture(GLenum target) const {
  return GetTexture(active_texture_unit_ + GL_TEXTURE0, target);
}

GLuint TextureContext::GetTexture(GLenum id, GLenum target) const {
  const GLuint index = id - GL_TEXTURE0;
  LOG_ALWAYS_FATAL_IF(index >= num_texture_units_);
  const Target t = MapTarget(target);
  const TextureUnit& unit = GetTextureUnit(t, index);
  return unit.texture;
}

GLenum TextureContext::GetGlobalTarget(GLenum id, GLenum target) const {
  const GLuint index = id - GL_TEXTURE0;
  LOG_ALWAYS_FATAL_IF(index >= (GLuint)num_texture_units_);
  const Target t = MapTarget(target);
  const TextureUnit& unit = GetTextureUnit(t, index);
  return unit.global_target;
}

TextureContext::TextureUnit&
TextureContext::GetTextureUnit(Target t, GLuint index) {
  return texture_units_[t][index];
}

const TextureContext::TextureUnit&
TextureContext::GetTextureUnit(Target t, GLuint index) const {
  return texture_units_[t][index];
}

void TextureContext::BindUnderlying(const TextureUnit& unit) {
  const GLuint global_name =
      context_->GetShareGroup()->GetTextureGlobalName(unit.texture);
  const GLenum global_target = unit.global_target;
  PASS_THROUGH(context_, BindTexture, global_target, global_name);
}

void TextureContext::ActiveTextureUnderlying(GLuint index) {
  const GLenum id = GL_TEXTURE0 + index;
  PASS_THROUGH(context_, ActiveTexture, id);
}

bool TextureContext::SetClientActiveTexture(GLenum texture) {
  const GLuint texture_unit = texture - GL_TEXTURE0;
  if (texture_unit >= num_texture_units_) {
    return false;
  }
  client_active_texture_unit_ = texture_unit;
  return true;
}

TextureContext::Target TextureContext::MapTarget(GLenum target) const {
  switch (target) {
    case GL_TEXTURE_2D:
      return kTexture2d;
    case GL_TEXTURE_CUBE_MAP:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
      return kTextureCubeMap;
    case GL_TEXTURE_EXTERNAL_OES:
      return kTextureExternal;
    default:
      LOG_ALWAYS_FATAL("Unsupported.");
  }
}

PointerContext::PointerContext(GlesContext* context)
  : context_(context),
    array_buffer_(0),
    array_buffer_size_(0),
    element_array_buffer_(0),
    element_array_buffer_size_(0),
    disable_gl_fixed_attribs_(emugl::GlesOptions::GLFixedAttribsEnabled()) {
  DLOG("Created pointer context for GLES1 context @ %p", context);
}

PointerContext::~PointerContext() {
  Release();
}

void PointerContext::Init(int num_pointers) {
  DLOG("Initialized pointer context for GLES1 context @ %p", context_);
  LOG_ALWAYS_FATAL_IF(array_buffer_ != 0);

  pointers_.resize(num_pointers);

  GLuint buffers[2];
  PASS_THROUGH(context_, GenBuffers, 2, buffers);
  array_buffer_ = buffers[0];
  element_array_buffer_ = buffers[1];
}

void PointerContext::Release() {
  if (array_buffer_) {
    GLuint buffers[2] = { array_buffer_, element_array_buffer_ };
    PASS_THROUGH(context_, DeleteBuffers, 2, buffers);
    array_buffer_ = 0;
    element_array_buffer_ = 0;
    array_buffer_size_ = 0;
    element_array_buffer_size_ = 0;
  }
}

void PointerContext::EnableArray(GLuint index) {
  if (index < pointers_.size()) {
    pointers_[index].enabled = true;
  }
}

void PointerContext::DisableArray(GLuint index) {
  if (index < pointers_.size()) {
    pointers_[index].enabled = false;
  }
}

bool PointerContext::IsArrayEnabled(GLuint index) const {
  if (index < pointers_.size()) {
    return pointers_[index].enabled;
  }
  return false;
}

void PointerContext::SetPointer(GLuint index, GLint size, GLenum type,
                                GLsizei stride, const GLvoid* pointer,
                                GLboolean normalized) {
  const GLuint buffer = context_->array_buffer_binding_;

  LOG_ALWAYS_FATAL_IF(index >= pointers_.size());
  PointerData& ptr = pointers_[index];
  ptr.size = size;
  ptr.type = type;
  ptr.stride = stride;
  ptr.normalize = normalized;
  ptr.buffer_name = buffer;
  ptr.pointer = pointer;
}

const PointerData* PointerContext::GetPointerData(GLuint index) const {
  if (index < pointers_.size()) {
    return &pointers_[index];
  }
  return NULL;
}

void PointerContext::SetPointers(const PointerDataVector& pointers) {
  LOG_ALWAYS_FATAL_IF(pointers_.size() != pointers.size());
  std::copy(pointers.begin(), pointers.end(), pointers_.begin());
}

void PointerContext::PrepareBuffersForDrawArrays(GLint first, GLsizei count) {
  BindPointers(first, first + count);
}

const GLvoid* PointerContext::PrepareBuffersForDrawElements(
    GLsizei count, GLenum type, const GLvoid* indices) {
  BufferDataPtr vbo =
      context_->GetBoundTargetBufferData(GL_ELEMENT_ARRAY_BUFFER);
  // There is no currently bound element array buffer, so copy the indices
  // into the PointerContext's element array buffer object and use that VBO
  // for the duration of this draw call.
  if (vbo == NULL) {
    const size_t size = GetTypeSize(type) * count;
    PASS_THROUGH(context_, BindBuffer, GL_ELEMENT_ARRAY_BUFFER,
                 element_array_buffer_);
    if (size > element_array_buffer_size_) {
      PASS_THROUGH(context_, BufferData, GL_ELEMENT_ARRAY_BUFFER, size, NULL,
                   GL_STREAM_DRAW);
      element_array_buffer_size_ = size;
    }
    PASS_THROUGH(context_, BufferSubData, GL_ELEMENT_ARRAY_BUFFER, 0, size,
                 indices);
  }

  // If we have any vertex attributes that are client-side, then we need to
  // calculate the range of the elements we are drawing so that we can copy the
  // client-data into VBOs.  The copying of the data is done in BindPointers.
  // If we have no client-side vertex attributes, then the range does not
  // matter.
  bool has_client_vertex_attribs = false;
  for (size_t i = 0; i < pointers_.size(); ++i) {
    if (!pointers_[i].enabled) {
      continue;
    }
    if (pointers_[i].buffer_name) {
      continue;
    }
    has_client_vertex_attribs = true;
    break;
  }

  if (has_client_vertex_attribs) {
    const GLvoid* data = indices;
    if (vbo != NULL) {
      const unsigned char* buffer_data = vbo->GetData();
      LOG_ALWAYS_FATAL_IF(buffer_data == NULL,
                          "Element array buffers must have data!");
      data = buffer_data + reinterpret_cast<uintptr_t>(indices);
    }
    const ElementRange range = GetElementRange(count, type, data);
    BindPointers(range.first, range.last + 1);
  } else {
    BindPointers(0, 0);
  }

  // If we have an element array VBO, then use the offset as specified.
  // Otherwise we have copied the client-data data into a VBO, so we
  // can use a zero (NULL) offset into the VBO instead.
  return vbo != NULL ? indices : NULL;
}

void PointerContext::BindPointers(GLint first, GLint last) {
  // Calculate data size for needed data in client side arrays.
  size_t size = 0;
  for (size_t index = 0; index < pointers_.size(); ++index) {
    PointerData& ptr = pointers_[index];
    if (!ptr.enabled) {
      continue;
    }
    if (disable_gl_fixed_attribs_) {
      LOG_ALWAYS_FATAL_IF(ptr.type == GL_FIXED,
                          "GL_FIXED type attribs not supported.");
    }

    // The vertex data is already stored in a VBO.  Bind the VBO now so
    // that we can correctly update the vertex attribute pointer to point
    // to it.
    if (ptr.buffer_name) {
      const ObjectGlobalName global_name =
          context_->GetShareGroup()->GetBufferGlobalName(ptr.buffer_name);
      PASS_THROUGH(context_, BindBuffer, GL_ARRAY_BUFFER, global_name);

      // TODO(crbug.com/482070): Convert any elements in the buffer data that
      // are of type GL_FIXED to GL_FLOAT and re-upload that data to the bound
      // buffer.
      if (ptr.type == GL_FIXED) {
        // We should be able to safely assume that ptr.size is [1,4] as
        // per the glVertexAttribPointer specs.  This allows us to just
        // use a local array instead of having to allocate a dynamic
        // array for doing the conversion.
        LOG_ALWAYS_FATAL_IF(ptr.size > 4);
        BufferDataPtr buffer =
            context_->GetShareGroup()->GetBufferData(ptr.buffer_name);
        const unsigned char* data = buffer->GetData();
        const size_t size = buffer->GetSize();
        const GLintptr stride =
            ptr.stride ? ptr.stride : ptr.size * GetTypeSize(ptr.type);

        for (size_t marker = reinterpret_cast<size_t>(ptr.pointer);
             marker < size; marker += stride) {
          GLfloat dst[4];
          const GLfixed* src = reinterpret_cast<const GLfixed*>(&data[marker]);
          for (int i = 0; i < ptr.size; ++i) {
            dst[i] = X2F(src[i]);
          }
          PASS_THROUGH(context_, BufferSubData, GL_ARRAY_BUFFER, marker,
                       ptr.size * sizeof(GLfloat), dst);
        }
      }

      PASS_THROUGH(context_, VertexAttribPointer, index, ptr.size,
                   ptr.type == GL_FIXED ? GL_FLOAT : ptr.type,
                   ptr.normalize, ptr.stride, ptr.pointer);
      continue;
    }

    size +=
      ptr.stride ? ptr.stride * last : GetTypeSize(ptr.type) * last * ptr.size;
  }

  // No client side array is used.
  if (size == 0) {
    return;
  }

  PASS_THROUGH(context_, BindBuffer, GL_ARRAY_BUFFER, array_buffer_);
  if (size > array_buffer_size_) {
    PASS_THROUGH(context_, BufferData, GL_ARRAY_BUFFER, size, NULL,
                 GL_STREAM_DRAW);
    array_buffer_size_ = size;
  }

  size_t offset = 0;
  for (size_t index = 0; index < pointers_.size(); ++index) {
    PointerData& ptr = pointers_[index];
    if (!ptr.enabled) {
      continue;
    }

    if (ptr.buffer_name) {
      continue;
    }

    const size_t stride =
      ptr.stride ? ptr.stride : GetTypeSize(ptr.type) * ptr.size;
    const size_t offset_first = stride * first;
    const size_t offset_last = stride * last;
    const unsigned char* data =
      reinterpret_cast<const unsigned char*>(ptr.pointer);

    // TODO(crbug.com/482070): Convert any elements in the buffer data that are
    // of type GL_FIXED to GL_FLOAT before copying it to our buffer object.
    if (ptr.type == GL_FIXED) {
      unsigned char* tmp = new unsigned char[offset_last];
      for (size_t marker = offset_first; marker < offset_last; marker += stride) {
        GLfloat* dst = reinterpret_cast<GLfloat*>(&tmp[marker]);
        const GLfixed* src = reinterpret_cast<const GLfixed*>(&data[marker]);
        for (int i = 0; i < ptr.size; ++i) {
          dst[i] = X2F(src[i]);
        }
      }
      data = tmp;
    }

    PASS_THROUGH(context_, BufferSubData, GL_ARRAY_BUFFER,
                 offset + offset_first, offset_last - offset_first,
                 data + offset_first);
    PASS_THROUGH(context_, VertexAttribPointer, index, ptr.size,
                 ptr.type == GL_FIXED ? GL_FLOAT : ptr.type,
                 ptr.normalize, ptr.stride, reinterpret_cast<void*>(offset));
    if (ptr.type == GL_FIXED) {
      delete[] data;
    }
    offset += offset_last;
  }
}
