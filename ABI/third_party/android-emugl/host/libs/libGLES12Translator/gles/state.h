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
#ifndef GRAPHICS_TRANSLATION_GLES_STATE_H_
#define GRAPHICS_TRANSLATION_GLES_STATE_H_

#include <map>
#include <utility>
#include <vector>

#include "vector.h"
#include "gles/dirtiable.h"
#include "gles/lazy_cache.h"
#include "gles/matrix_stack.h"
#include "gles/texture_data.h"

class GlesContext;
class ProgramContext;

// This tuple is used to map the VertexAttributeKey enumeration to a specific
// vertex attribute in the vertex shader for emulation.
// N.B.  Position has to be attribute 0 otherwise Pepper complains.
#define VERTEX_ATTRIBUTE_KEY_TUPLE                                    \
  VERTEX_ATTRIBUTE_KEY(kPositionVertexAttribute, "a_position")        \
  VERTEX_ATTRIBUTE_KEY(kColorVertexAttribute, "a_color")              \
  VERTEX_ATTRIBUTE_KEY(kNormalVertexAttribute, "a_normal")            \
  VERTEX_ATTRIBUTE_KEY(kPointSizeVertexAttribute, "a_point_size")     \
  VERTEX_ATTRIBUTE_KEY(kTexCoord0VertexAttribute, "a_texcoord_0")     \
  VERTEX_ATTRIBUTE_KEY(kTexCoord1VertexAttribute, "a_texcoord_1")     \
  VERTEX_ATTRIBUTE_KEY(kTexCoord2VertexAttribute, "a_texcoord_2")     \
  VERTEX_ATTRIBUTE_KEY(kTexCoord3VertexAttribute, "a_texcoord_3")     \
  VERTEX_ATTRIBUTE_KEY(kTexCoord4VertexAttribute, "a_texcoord_4")     \
  VERTEX_ATTRIBUTE_KEY(kTexCoord5VertexAttribute, "a_texcoord_5")     \
  VERTEX_ATTRIBUTE_KEY(kTexCoord6VertexAttribute, "a_texcoord_6")     \
  VERTEX_ATTRIBUTE_KEY(kTexCoord7VertexAttribute, "a_texcoord_7")     \
  VERTEX_ATTRIBUTE_KEY(kWeightVertexAttribute, "a_weight")            \
  VERTEX_ATTRIBUTE_KEY(kMatrixIndexVertexAttribute, "a_matrix_index")

#define VERTEX_ATTRIBUTE_KEY(enum, name) enum,
enum VertexAttributeKey {
  VERTEX_ATTRIBUTE_KEY_TUPLE
  kNumVertexAttributeKeys
};
#undef VERTEX_ATTRIBUTE_KEY

// This tuple is used to map the UniformKey enumeration to a uniform location
// name in the generated shaders for emulation.
#define UNIFORM_KEY_TUPLE                                                     \
  UNIFORM_KEY(kAlphaTestConstantUniform, "u_alpha_test_constant")             \
  UNIFORM_KEY(kAmbientLightUniform, "u_ambient")                              \
  UNIFORM_KEY(kClipPlaneUniform, "u_clip_plane[%d]")                          \
  UNIFORM_KEY(kColorUniform, "u_color")                                       \
  UNIFORM_KEY(kFogColorUniform, "u_fog_color")                                \
  UNIFORM_KEY(kFogDensityUniform, "u_fog_density")                            \
  UNIFORM_KEY(kFogEndUniform, "u_fog_end")                                    \
  UNIFORM_KEY(kFogStartUniform, "u_fog_start")                                \
  UNIFORM_KEY(kLightAmbientUniform, "u_light[%d].ambient")                    \
  UNIFORM_KEY(kLightConstAttenuationUniform, "u_light[%d].const_attenuation") \
  UNIFORM_KEY(kLightDiffuseUniform, "u_light[%d].diffuse")                    \
  UNIFORM_KEY(kLightDirectionUniform, "u_light[%d].direction")                \
  UNIFORM_KEY(kLightLinearAttenuationUniform,                                 \
              "u_light[%d].linear_attenuation")                               \
  UNIFORM_KEY(kLightPositionUniform, "u_light[%d].position")                  \
  UNIFORM_KEY(kLightQuadraticAttenuationUniform,                              \
              "u_light[%d].quadratic_attenuation")                            \
  UNIFORM_KEY(kLightSpecularUniform, "u_light[%d].specular")                  \
  UNIFORM_KEY(kLightSpotCutoffUniform, "u_light[%d].spot_cutoff")             \
  UNIFORM_KEY(kLightSpotExponentUniform, "u_light[%d].spot_exponent")         \
  UNIFORM_KEY(kMaterialAmbientUniform, "u_material.ambient")                  \
  UNIFORM_KEY(kMaterialDiffuseUniform, "u_material.diffuse")                  \
  UNIFORM_KEY(kMaterialEmissionUniform, "u_material.emission")                \
  UNIFORM_KEY(kMaterialShininessUniform, "u_material.shininess")              \
  UNIFORM_KEY(kMaterialSpecularUniform, "u_material.specular")                \
  UNIFORM_KEY(kModelViewInverseTransposeMatrixUniform,                        \
              "u_mv_inverse_transpose_matrix")                                \
  UNIFORM_KEY(kModelViewMatrixUniform, "u_mv_matrix")                         \
  UNIFORM_KEY(kNormalUniform, "u_normal")                                     \
  UNIFORM_KEY(kPaletteMatrixUniform, "u_palette_matrix[%d]")                  \
  UNIFORM_KEY(kPaletteInverseMatrixUniform, "u_palette_inv_matrix[%d]")       \
  UNIFORM_KEY(kPointSizeAttenuationUniform, "u_point_size_attenuation")       \
  UNIFORM_KEY(kPointSizeMaxUniform, "u_point_size_max")                       \
  UNIFORM_KEY(kPointSizeMinUniform, "u_point_size_min")                       \
  UNIFORM_KEY(kProjectionMatrixUniform, "u_p_matrix")                         \
  UNIFORM_KEY(kTextureEnvColorUniform, "u_texenv_const[%d]")                  \
  UNIFORM_KEY(kTextureEnvScaleUniform, "u_texenv_scale[%d]")                  \
  UNIFORM_KEY(kTextureMatrixUniform, "u_texture_matrix[%d]")                  \
  UNIFORM_KEY(kTextureSamplerUniform, "u_sampler_%d")

#define UNIFORM_KEY(enum, name) enum,
enum UniformKey {
  UNIFORM_KEY_TUPLE
  kNumUniformKeys
};
#undef UNIFORM_KEY


// Structure to store light source parameters.  See es_full_spec.1.1.12.pdf
// section 2.12.1 table 2.8 for information about the parameters.
struct Light {
  Light();
  bool IsSpot() const;
  bool IsPositional() const;
  bool IsDirectional() const;
  bool ShouldAttenuate() const;

  emugl::Vector ambient;
  emugl::Vector diffuse;
  emugl::Vector specular;
  emugl::Vector position;
  emugl::Vector direction;
  float spot_exponent;
  float spot_cutoff;
  float const_attenuation;
  float linear_attenuation;
  float quadratic_attenuation;
};

// Structure to store material parameters.  See es_full_spec.1.1.12.pdf section
// 2.12.1 table 2.8 for information about the parameters.
struct Material {
  Material();
  emugl::Vector ambient;
  emugl::Vector diffuse;
  emugl::Vector specular;
  emugl::Vector emission;
  float shininess;
};

// Structure to store fog parameters.  See es_full_spec.1.1.12.pdf section 3.8.
// The names for these parameters correspond to the GLenums that are used to
// set them through glFogf and glFogfv.
struct Fog {
  Fog();
  GLenum mode;
  emugl::Vector color;
  float density;
  float start;
  float end;
};

struct TexEnv {
  TexEnv();
  int mode;
  int combine_rgb;
  int combine_alpha;
  int src_rgb[3];
  int src_alpha[3];
  int operand_rgb[3];
  int operand_alpha[3];
  emugl::Vector color;
  float rgb_scale;
  float alpha_scale;
};

struct TexGen {
  TexGen();
  GLenum mode;
  GLboolean enabled;
};

struct AlphaTest {
  AlphaTest();
  GLenum func;
  float value;
};

struct PointParameters {
  PointParameters();
  // Minimum size of the point
  float size_min;
  // Maximum size of the point
  float size_max;
  // Sets the size at which the point will fade to an alpha value of zero
  float threshold_size;
  // Controls how the point size is scaled with distance from the eye
  float attenuation[3];
  // Current point size. Constant size used when no array is set.
  float current_size;
};

struct PointerData {
 public:
  PointerData();

  bool enabled;
  GLint size;
  GLenum type;
  GLsizei stride;
  GLboolean normalize;
  GLuint buffer_name;
  const GLvoid* pointer;
};

// Helps with the implementation of StateCache by implementing the Load
// functionality it needs.
// The implementation cannot be inlined here as it requires classes which have
// not yet been defined, and which depend on StateCache being defined first.
class StateCacheLoaderHelper {
 public:
  static void Load(GLenum value, GLboolean* data);
  static void Load(GLenum value, GLfloat* data);
  static void Load(GLenum value, GLint* data);
  static void Load(GLenum value, GLuint* data);
  static void Load(GLenum value,
                   GLboolean& data);              // NOLINT(runtime/references)
  static void Load(GLenum value, GLfloat& data);  // NOLINT(runtime/references)
  static void Load(GLenum value, GLint& data);    // NOLINT(runtime/references)
  static void Load(GLenum value, GLuint& data);   // NOLINT(runtime/references)

 protected:
  StateCacheLoaderHelper();
};

// Defines a lazy-loaded cache for GLES state.
//
// The state is loaded from the underlying implementation if not yet known.
// State can also be set into the cache directly, bypassing the need for the
// query in cases where the application sets the state away from the default.
//
// This cache saves substantial time per query given the latency needed to
// simply pass the call through to the underlying implementation and wait for it
// to return. Loading the cache lazily also saves on time at startup given that
// most state may never be queried.
template <typename T, GLenum V>
class StateCache {
 public:
  enum { kValue = V };
  T& Mutate() { return cache_.Mutate(); }
  const T& Get() const { return cache_.Get(); }

 private:
  static void Load(T& t) {  // NOLINT(runtime/references)
    StateCacheLoaderHelper::Load(kValue, t);
  }
  LazyCache<T, StateCache<T, V>::Load> cache_;
};

// A helper class to draw fullscreen quads.
class FullscreenQuad {
 public:
  explicit FullscreenQuad(GlesContext* context);
  ~FullscreenQuad();

  void Draw(GLuint texture, bool flip_v);

 private:
  GlesContext* context_;
  GLuint program_;
  GLuint buffer_;
  GLuint position_attrib_idx_;
  GLuint texcoord_attrib_idx_;
  GLuint sampler_uniform_idx_;

  FullscreenQuad(const FullscreenQuad&);
  FullscreenQuad& operator=(const FullscreenQuad&);
};

// Stores all context data that requires emulation.  We separate this
// to implement a dependency mechanism using C++ type checking to know
// whenever uniform-specific context has changed (and requires us to
// update our shader uniforms.)
class UniformContext {
 public:
  enum {
    kMaxLights = 8,
    kMaxClipPlanes = 6,
    kMaxTextureUnits = 8,
    kMaxPaletteMatricesOES = 9,
    kMaxVertexUnitsOES = 4,
  };

  explicit UniformContext(GlesContext* context);
  void Init(int num_texture_units);

  void Bind(ProgramContext* program);

  void Enable(GLenum cap);
  void Disable(GLenum cap);
  GLboolean IsEnabled(GLenum cap) const;

  bool SetMatrixMode(GLenum matrix_mode);
  GLenum GetMatrixMode() const { return matrix_mode_; }
  bool SetActiveTexture(GLenum texture_id);

  const emugl::Matrix& GetModelViewMatrix() const;
  emugl::Matrix& MutateModelViewMatrix();

  const emugl::Matrix& GetProjectionMatrix() const;
  emugl::Matrix& MutateProjectionMatrix();

  const emugl::Matrix& GetTextureMatrix() const;
  const emugl::Matrix& GetTextureMatrixByStage(GLint stage) const;
  emugl::Matrix& MutateTextureMatrixByStage(GLint stage);

  bool ActiveMatrixPush();
  bool ActiveMatrixPop();

  emugl::Matrix& MutateActiveMatrix();
  const emugl::Matrix& GetActiveMatrix() const;

  bool SetCurrentPaletteMatrix(GLuint index);
  GLuint GetCurrentPaletteMatrix() const { return current_palette_matrix_; }
  const emugl::Matrix& GetPaletteMatrix() const;
  emugl::Matrix& MutatePaletteMatrix();

  TexEnv& MutateActiveTexEnv();
  const TexEnv& GetActiveTexEnv() const;

  TexGen& MutateActiveTexGen();
  const TexGen& GetActiveTexGen() const;

  emugl::Vector* MutateClipPlane(GLenum id);
  const emugl::Vector* GetClipPlane(GLenum id) const;

  TexEnv* MutateTexEnv(GLenum id);
  const TexEnv* GetTexEnv(GLenum id) const;

  TexGen* MutateTexGen(GLenum id);
  const TexGen* GetTexGen(GLenum id) const;

  Light* MutateLight(GLenum id);
  const Light* GetLight(GLenum id) const;

  PointParameters& MutatePointParameters() {
    return point_parameters_.Mutate();
  }

  const PointParameters& GetPointParameters() const {
    return point_parameters_.Get();
  }

  AlphaTest& MutateAlphaTest() { return alpha_test_.Mutate(); }
  const AlphaTest& GetAlphaTest() const { return alpha_test_.Get(); }

  Material& MutateMaterial() { return material_.Mutate(); }
  const Material& GetMaterial() const { return material_.Get(); }

  Fog& MutateFog() { return fog_.Mutate(); }
  const Fog& GetFog() const { return fog_.Get(); }

  emugl::Vector& MutateColor() { return color_.Mutate(); }
  const emugl::Vector& GetColor() const { return color_.Get(); }

  emugl::Vector& MutateNormal() { return normal_.Mutate(); }
  const emugl::Vector& GetNormal() const { return normal_.Get(); }

  emugl::Vector& MutateAmbient() { return ambient_.Mutate(); }
  const emugl::Vector& GetAmbient() const { return ambient_.Get(); }

 private:
  const Dirtiable<MatrixStack>* GetActiveMatrixStackInternal() const;

  GlesContext* context_;
  GLuint program_object_;
  GLenum matrix_mode_;
  GLint active_texture_unit_;
  GLint current_palette_matrix_;

  Dirtiable<AlphaTest> alpha_test_;
  Dirtiable<emugl::Vector> ambient_;
  Dirtiable<emugl::Vector> clip_planes_[kMaxClipPlanes];
  Dirtiable<emugl::Vector> color_;
  Dirtiable<Fog> fog_;
  Dirtiable<Light> lights_[kMaxLights];
  Dirtiable<Material> material_;
  Dirtiable<MatrixStack> modelview_matrix_stack_;
  Dirtiable<emugl::Vector> normal_;
  Dirtiable<PointParameters> point_parameters_;
  Dirtiable<MatrixStack> projection_matrix_stack_;
  Dirtiable<TexEnv> texenv_[kMaxTextureUnits];
  Dirtiable<TexGen> texgen_[kMaxTextureUnits];
  Dirtiable<MatrixStack> texture_matrix_stack_[kMaxTextureUnits];
  Dirtiable<emugl::Matrix> palette_matrices_[kMaxPaletteMatricesOES];
};

// Stores information about the current gles_translation program that is
// being used to emulate the GLES 1 draw state.
class ProgramContext {
 public:
  ProgramContext(GlesContext* context, GLuint program);
  void Bind();
  void Delete();

  GLuint GetProgramObject() const { return program_object_; }
  GLint GetUniformLocation(UniformKey key);
  GLint GetUniformLocation(UniformKey key, int index);

  void BindUniform(const int& value, UniformKey key);
  void BindUniform(const int& value, UniformKey key, int index);
  void BindUniform(const float& value, UniformKey key);
  void BindUniform(const float& value, UniformKey key, int index);
  void BindUniform(const float (&values)[3], UniformKey key);
  void BindUniform(const emugl::Vector& vector, UniformKey key);
  void BindUniform(const emugl::Vector& vector, UniformKey key, int index);
  void BindUniform(const emugl::Matrix& matrix, UniformKey key);
  void BindUniform(const emugl::Matrix& matrix, UniformKey key, int index);

  void BindUniform(const Fog& fog);
  void BindUniform(const Light& light, int index);
  void BindUniform(const Material& material);
  void BindUniform(const PointParameters& point_parameters);
  void BindUniform(const TexEnv& texenv, int index);

 private:
  enum {
    kScalarUniform = -1
  };

  // Uniform locations are keyed by their name (eg. u_mv_matrix) and array
  // index (eg. u_light[0]).  Uniforms that are not arrays have an implied
  // index of kScalarUniform.
  typedef std::pair<UniformKey, int> UniformKeyIdx;
  typedef std::map<UniformKeyIdx, GLint> UniformLocationMap;

  GlesContext* context_;
  GLuint program_object_;
  UniformLocationMap uniform_location_map_;
};

class TextureContext {
  struct TextureUnit;
 public:
  explicit TextureContext(GlesContext* context);
  ~TextureContext();

  void Init(int num_texture_units, int max_texture_size);
  void DeleteTexture(GLuint texture);

  GLint GetMaxLevels() const { return max_texture_levels_; }
  TextureDataPtr GetDefaultTextureData(GLenum target);

  GLenum GetActiveTexture() const { return GL_TEXTURE0 + active_texture_unit_; }
  bool SetActiveTexture(GLenum id);

  bool BindTextureToTarget(const TextureDataPtr& tex, GLenum target);
  void SetTargetTexture(GLenum target, GLuint texture, GLenum global_target);

  GLenum EnsureCorrectBinding(GLenum target);
  void RestoreBinding(GLenum target);

  void PrepareTextures(bool gles11, bool uses_external_as_2d);
  void RestoreTextures();

  void Enable(GLenum target);
  void Disable(GLenum target);
  bool IsEnabled(GLenum target) const;
  bool IsEnabled(GLenum id, GLenum target) const;

  GLuint GetBoundTexture(GLenum target) const;
  GLuint GetTexture(GLenum id, GLenum target) const;
  GLenum GetGlobalTarget(GLenum id, GLenum target) const;

  bool SetClientActiveTexture(GLenum texture);
  GLenum GetClientActiveTexture() const {
    return GL_TEXTURE0 + client_active_texture_unit_;
  }
  GLuint GetClientActiveTextureCoordAttrib() const {
    return kTexCoord0VertexAttribute + client_active_texture_unit_;
  }
  static GLuint GetTextureCoordAttrib(GLenum texture) {
    return kTexCoord0VertexAttribute + texture - GL_TEXTURE0;
  }

 private:
  enum Target {
    kTexture2d,
    kTextureCubeMap,
    kTextureExternal,
    kNumTargets
  };

  struct TextureUnit {
    TextureUnit() : texture(0), global_target(0), enabled(GL_FALSE) {
    }
    GLuint texture;
    GLenum global_target;
    GLboolean enabled;
  };

  Target MapTarget(GLenum target) const;
  TextureUnit& GetTextureUnit(Target t, GLuint index);
  const TextureUnit& GetTextureUnit(Target t, GLuint index) const;
  void BindUnderlying(const TextureUnit& texture_unit);
  void ActiveTextureUnderlying(GLuint index);

  // The maximum number of texture mipmap levels supported. Computed from the
  // maximum supported texture size of the underlying implementation.
  GlesContext* context_;
  GLint max_texture_levels_;
  GLuint num_texture_units_;
  GLuint active_texture_unit_;
  GLenum client_active_texture_unit_;
  std::vector<TextureUnit> texture_units_[kNumTargets];

  static const GLenum supported_texture_targets_[kNumTargets];

  // Texture information about the default texture (ie. texture 0).  This
  // information is not shared between contexts.
  std::vector<TextureDataPtr> default_textures_;

  TextureContext(const TextureContext&);
  TextureContext& operator=(const TextureContext&);
};

class PointerContext {
 public:
  typedef std::vector<PointerData> PointerDataVector;

  explicit PointerContext(GlesContext* context);
  ~PointerContext();

  void Init(int num_pointers);
  void Release();

  void EnableArray(GLuint index);
  void DisableArray(GLuint index);
  bool IsArrayEnabled(GLuint index) const;

  const PointerData* GetPointerData(GLuint index) const;
  void SetPointer(GLuint index, GLint size, GLenum type, GLsizei stride,
                  const GLvoid* pointer, GLboolean normalized = GL_FALSE);

  void PrepareBuffersForDrawArrays(GLint first, GLsizei count);
  const GLvoid* PrepareBuffersForDrawElements(GLsizei count, GLenum type,
                                              const GLvoid* indices);

  const PointerDataVector& GetPointers() const { return pointers_; }
  void SetPointers(const PointerDataVector& pointers);

 private:
  void BindPointers(GLint first, GLint last);

  GlesContext* context_;
  PointerDataVector pointers_;

  GLuint array_buffer_;
  size_t array_buffer_size_;
  GLuint element_array_buffer_;
  size_t element_array_buffer_size_;

  bool disable_gl_fixed_attribs_;

  PointerContext(const PointerContext&);
  PointerContext& operator=(const PointerContext&);
};

#endif  // GRAPHICS_TRANSLATION_GLES_STATE_H_
