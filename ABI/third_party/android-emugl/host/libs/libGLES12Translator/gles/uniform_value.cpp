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

#include "gles/uniform_value.h"

#include <algorithm>
#include "common/alog.h"
#include "gles/debug.h"
#include "gles/gles_context.h"
#include "gles/macros.h"

UniformValue::UniformValue(GLenum type, bool is_array, GLint array_size)
    : type_(type), is_array_(is_array), array_size_(array_size) {
  InitData(reinterpret_cast<GLfloat*>(NULL));
}

UniformValue::UniformValue(GLenum type, bool is_array, GLint array_size,
                           const GLfloat* value)
    : type_(type), is_array_(is_array), array_size_(array_size) {
  InitData(value);
  LOG_ALWAYS_FATAL_IF(data_type_ != DATA_FLOAT && data_type_ != DATA_BOOL &&
                      data_type_ != DATA_MATRIX);
}

UniformValue::UniformValue(GLenum type, bool is_array, GLint array_size,
                           const GLint* value)
    : type_(type), is_array_(is_array), array_size_(array_size) {
  InitData(value);
  LOG_ALWAYS_FATAL_IF(data_type_ != DATA_INT && data_type_ != DATA_BOOL &&
                      data_type_ != DATA_SAMPLER);
}

UniformValue::~UniformValue() {
  delete[] value_;
}

template<typename T>
void UniformValue::InitData(const T* initial_value) {
  switch (type_) {
    case GL_FLOAT:
      data_type_ = DATA_FLOAT;
      components_ = 1;
      break;
    case GL_FLOAT_VEC2:
      data_type_ = DATA_FLOAT;
      components_ = 2;
      break;
    case GL_FLOAT_VEC3:
      data_type_ = DATA_FLOAT;
      components_ = 3;
      break;
    case GL_FLOAT_VEC4:
      data_type_ = DATA_FLOAT;
      components_ = 4;
      break;
    case GL_INT:
      data_type_ = DATA_INT;
      components_ = 1;
      break;
    case GL_INT_VEC2:
      data_type_ = DATA_INT;
      components_ = 2;
      break;
    case GL_INT_VEC3:
      data_type_ = DATA_INT;
      components_ = 3;
      break;
    case GL_INT_VEC4:
      data_type_ = DATA_INT;
      components_ = 4;
      break;
    case GL_BOOL:
      data_type_ = DATA_BOOL;
      components_ = 1;
      break;
    case GL_BOOL_VEC2:
      data_type_ = DATA_BOOL;
      components_ = 2;
      break;
    case GL_BOOL_VEC3:
      data_type_ = DATA_BOOL;
      components_ = 3;
      break;
    case GL_BOOL_VEC4:
      data_type_ = DATA_BOOL;
      components_ = 4;
      break;
    case GL_FLOAT_MAT2:
      data_type_ = DATA_MATRIX;
      components_ = 2 * 2;
      break;
    case GL_FLOAT_MAT3:
      data_type_ = DATA_MATRIX;
      components_ = 3 * 3;
      break;
    case GL_FLOAT_MAT4:
      data_type_ = DATA_MATRIX;
      components_ = 4 * 4;
      break;
    case GL_SAMPLER_2D:
    case GL_SAMPLER_CUBE:
      data_type_ = DATA_SAMPLER;
      components_ = 1;
      break;
    default:
      LOG_ALWAYS_FATAL("Unknown uniform type %s", GetEnumString(type_));
  }

  LOG_ALWAYS_FATAL_IF(array_size_ < 0, "Uniform array size is negative: %d",
                      array_size_);

  element_bytes_size_ = components_ *
      (IsFloatData() ? sizeof(GLfloat) : sizeof(GLint));

  value_ = new char[GetStorageBytesSize()];

  if (initial_value) {
    if (data_type_ == DATA_BOOL) {
      for (int i = 0; i < array_size_; ++i) {
        SetBoolElementFrom(i, initial_value + i);
      }
    } else {
      memcpy(value_, initial_value, GetStorageBytesSize());
    }
  } else {
    ResetData();
  }
}

void UniformValue::ResetData() {
  memset(value_, 0, GetStorageBytesSize());
}

int UniformValue::GetArrayPos(int idx) const {
  LOG_ALWAYS_FATAL_IF(idx < 0 || idx >= array_size_);
  return idx * element_bytes_size_;
}

bool UniformValue::CopyFrom(const UniformValue& src) {
  ResetData();
  const int size_to_copy = std::min(array_size_, src.array_size_);
  for (int i = 0; i < size_to_copy; ++i) {
    if (!CopyElementFrom(i, src, i)) {
      // When data is not convertible, the first call to CopyElementFrom()
      // will fail, leaving destination object filled with zeroes.
      // This should be OK as GLES2 is silent on this topic.
      return false;
    }
  }
  return true;
}

bool UniformValue::CopyElementFrom(int dst_idx, const UniformValue& src,
                                   int src_idx) {
  const int dst_pos = GetArrayPos(dst_idx);
  const int src_pos = src.GetArrayPos(src_idx);

  if (components_ != src.components_) {
    // GLES2 prescribes an error "if the size indicated in the name of
    // the Uniform* command used does not match the size of the uniform
    // declared in the shader". The components size of sampler uniform
    // is assumed to be 1, as only glUniform1iv() can be used there.
#ifdef ENABLE_API_LOGGING
    ALOGW("Unable to convert uniform value. Src=%d/%d. Dst=%d/%d",
          src.data_type_, src.components_, data_type_, components_);
#endif
    return false;
  }

  if (data_type_ == src.data_type_) {
    // Same general type and components size - copy the element as is.
    memcpy(value_ + dst_pos, src.value_ + src_pos, element_bytes_size_);
    return true;
  }

  // The rest of the code performs data type conversions.

  if (data_type_ == DATA_MATRIX || src.data_type_ == DATA_MATRIX) {
    // GLES2 does not allow conversion between matrix and non-matrix types.
#ifdef ENABLE_API_LOGGING
    ALOGW("Unable to convert uniform value. Src=%d/%d. Dst=%d/%d",
          src.data_type_, src.components_, data_type_, components_);
#endif
    return false;
  }

  if (src.data_type_ == DATA_BOOL) {
    // Bool data can be converted to int or float.
    if (data_type_ == DATA_SAMPLER) {
#ifdef ENABLE_API_LOGGING
      ALOGW("Unable to convert uniform value. Src=%d/%d. Dst=%d/%d",
            src.data_type_, src.components_, data_type_, components_);
#endif
      return false;
    }
    LOG_ALWAYS_FATAL_IF(data_type_ != DATA_INT &&
                        data_type_ != DATA_FLOAT);
    if (IsFloatData()) {
      src.StoreBoolElementTo(
          reinterpret_cast<GLfloat*>(value_ + dst_pos), src_idx);
    } else {
      src.StoreBoolElementTo(
          reinterpret_cast<GLint*>(value_ + dst_pos), src_idx);
    }
    return true;
  }

  if (data_type_ == DATA_BOOL) {
    // Int or float data can be converted to bool.
    if (src.data_type_ == DATA_SAMPLER) {
#ifdef ENABLE_API_LOGGING
      ALOGW("Unable to convert uniform value. Src=%d/%d. Dst=%d/%d",
            src.data_type_, src.components_, data_type_, components_);
#endif
      return false;
    }
    LOG_ALWAYS_FATAL_IF(src.data_type_ != DATA_INT &&
                        src.data_type_ != DATA_FLOAT);
    if (src.IsFloatData()) {
      SetBoolElementFrom(
          dst_idx, reinterpret_cast<const GLfloat*>(src.value_ + src_pos));
    } else {
      SetBoolElementFrom(
          dst_idx, reinterpret_cast<const GLint*>(src.value_ + src_pos));
    }
    return true;
  }

  if (src.data_type_ == DATA_SAMPLER) {
    // Sampler data can be converted to int.
    if (IsFloatData()) {
#ifdef ENABLE_API_LOGGING
      ALOGW("Unable to convert uniform value. Src=%d/%d. Dst=%d/%d",
            src.data_type_, src.components_, data_type_, components_);
#endif
      return false;
    }
    LOG_ALWAYS_FATAL_IF(data_type_ != DATA_INT);
    memcpy(value_ + dst_pos, src.value_ + src_pos, element_bytes_size_);
    return true;
  }

  if (data_type_ == DATA_SAMPLER) {
    // Int data can be converted to sampler data.
    if (src.IsFloatData()) {
#ifdef ENABLE_API_LOGGING
      ALOGW("Unable to convert uniform value. Src=%d/%d. Dst=%d/%d",
            src.data_type_, src.components_, data_type_, components_);
#endif
      return false;
    }
    LOG_ALWAYS_FATAL_IF(src.data_type_ != DATA_INT);
    memcpy(value_ + dst_pos, src.value_ + src_pos, element_bytes_size_);
    return true;
  }

  // No other conversions are allowed by GLES2.
#ifdef ENABLE_API_LOGGING
  ALOGW("Unable to convert uniform value. Src=%d/%d. Dst=%d/%d",
        src.data_type_, src.components_, data_type_, components_);
#endif
  return false;
}

template<typename T>
void UniformValue::StoreBoolElementTo(T* dst, int idx) const {
  LOG_ALWAYS_FATAL_IF(data_type_ != DATA_BOOL);
  const GLint* src = reinterpret_cast<const GLint*>(
      value_ + GetArrayPos(idx));
  for (int i = 0; i < components_; ++i) {
    dst[i] = (src[i] != 0);
  }
}

template<typename T>
void UniformValue::SetBoolElementFrom(int idx, const T* src) {
  LOG_ALWAYS_FATAL_IF(data_type_ != DATA_BOOL);
  GLint* dst = reinterpret_cast<GLint*>(value_ + GetArrayPos(idx));
  for (int i = 0; i < components_; ++i) {
    dst[i] = (src[i] != 0);
  }
}

bool UniformValue::StoreAsFloatTo(GLfloat* value) const {
  if (data_type_ == DATA_BOOL) {
    for (int i = 0; i < array_size_; ++i) {
      StoreBoolElementTo(value + i * components_, i);
    }
    return true;
  }

  if (!IsFloatData()) {
    // Int or sampler. Only bool or float-based data can be requested here.
    return false;
  }

  memcpy(value, value_, GetStorageBytesSize());
  return true;
}

bool UniformValue::StoreAsIntTo(GLint* value) const {
  if (data_type_ == DATA_BOOL) {
    for (int i = 0; i < array_size_; ++i) {
      StoreBoolElementTo(value + i * components_, i);
    }
    return true;
  }

  if (IsFloatData()) {
    // Float or matrix. However, only bool and sampler conversions are allowed
    // for int type by GLES2.
    return false;
  }

  memcpy(value, value_, GetStorageBytesSize());
  return true;
}

void UniformValue::SetOnProgram(GlesContext* ctx,
                                GLuint location) const {
  if (!array_size_) {
    return;
  }
  const GLint* int_value = reinterpret_cast<const GLint*>(value_);
  const GLfloat* float_value = reinterpret_cast<const GLfloat*>(value_);
  switch (type_) {
    case GL_FLOAT:
      PASS_THROUGH(ctx, Uniform1fv, location, array_size_, float_value);
      break;
    case GL_FLOAT_VEC2:
      PASS_THROUGH(ctx, Uniform2fv, location, array_size_, float_value);
      break;
    case GL_FLOAT_VEC3:
      PASS_THROUGH(ctx, Uniform3fv, location, array_size_, float_value);
      break;
    case GL_FLOAT_VEC4:
      PASS_THROUGH(ctx, Uniform4fv, location, array_size_, float_value);
      break;
    case GL_INT:
    case GL_BOOL:
    case GL_SAMPLER_2D:
    case GL_SAMPLER_CUBE:
      PASS_THROUGH(ctx, Uniform1iv, location, array_size_, int_value);
      break;
    case GL_INT_VEC2:
    case GL_BOOL_VEC2:
      PASS_THROUGH(ctx, Uniform2iv, location, array_size_, int_value);
      break;
    case GL_INT_VEC3:
    case GL_BOOL_VEC3:
      PASS_THROUGH(ctx, Uniform3iv, location, array_size_, int_value);
      break;
    case GL_INT_VEC4:
    case GL_BOOL_VEC4:
      PASS_THROUGH(ctx, Uniform4iv, location, array_size_, int_value);
      break;
    case GL_FLOAT_MAT2:
      PASS_THROUGH(ctx, UniformMatrix2fv, location, array_size_,
                   GL_FALSE, float_value);
      break;
    case GL_FLOAT_MAT3:
      PASS_THROUGH(ctx, UniformMatrix3fv, location, array_size_,
                   GL_FALSE, float_value);
      break;
    case GL_FLOAT_MAT4:
      PASS_THROUGH(ctx, UniformMatrix4fv, location, array_size_,
                   GL_FALSE, float_value);
      break;
    default:
      LOG_ALWAYS_FATAL("Unknown uniform type %s", GetEnumString(type_));
  }
}
