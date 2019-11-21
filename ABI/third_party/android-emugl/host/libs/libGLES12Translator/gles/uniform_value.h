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
#ifndef GRAPHICS_TRANSLATION_GLES_UNIFORM_VALUE_H_
#define GRAPHICS_TRANSLATION_GLES_UNIFORM_VALUE_H_

#include <GLES/gl.h>

class GlesContext;

// Encapsulates value of one active uniform, used for caching uniform data.
// Per GLES2 spec, the following data types are supported:
//   1. GL_FLOAT, GL_FLOAT_VEC2, GL_FLOAT_VEC3, GL_FLOAT_VEC4
//   2. GL_INT, GL_INT_VEC2, GL_INT_VEC3, GL_INT_VEC4
//   3. GL_BOOL, GL_BOOL_VEC2, GL_BOOL_VEC3, GL_BOOL_VEC4
//   4. GL_FLOAT_MAT2, GL_FLOAT_MAT3, GL_FLOAT_MAT4
//   5. GL_SAMPLER_2D, GL_SAMPLER_CUBE
// Allowed implicit data conversions:
//   1. Filling in parts of arrays
//   2. Conversions between GL_INT* and GL_BOOL*
//   3. Conversions between GL_FLOAT* and GL_BOOL*
//   4. Conversions between GL_INT and GL_SAMPLER*
// Disallowed implicit data conversions:
//   1. Any changes of component size (for example, VEC2 vs VEC3)
//   2. Any type conversions not explicitly allowed
// TODO(crbug.com/441931): Add a unit test for UniformValue.
class UniformValue {
 public:
  UniformValue(GLenum type, bool is_array, GLint array_size);
  UniformValue(GLenum type, bool is_array, GLint array_size,
               const GLfloat* value);
  UniformValue(GLenum type, bool is_array, GLint array_size,
               const GLint* value);
  ~UniformValue();

  GLenum GetType() const { return type_; }
  bool IsArray() const { return is_array_; }
  GLint GetArraySize() const { return array_size_; }

  // Copies one element of a uniform. Performs necessary data conversions.
  // Returns false if data conversion is not possible. Index parameters
  // refer to positions within arrays; use zero if this is not an array.
  bool CopyElementFrom(int dst_idx, const UniformValue& src, int src_idx);

  // Copies all elements of a uniform. Performs necessary data conversions.
  // Returns false if data conversion is not possible.
  bool CopyFrom(const UniformValue& src);

  // Sets uniform's value at the given location on the current program.
  void SetOnProgram(GlesContext* ctx, GLuint global_location) const;

  bool StoreAsFloatTo(GLfloat* value) const;
  bool StoreAsIntTo(GLint* value) const;

 private:
  enum DataType {
    DATA_INT,
    DATA_BOOL,
    DATA_FLOAT,
    DATA_MATRIX,
    DATA_SAMPLER,
  };

  // Copy and assignment are not supported.
  UniformValue(const UniformValue& src);
  UniformValue& operator=(const UniformValue& src);

  template<typename T> void InitData(const T* initial_value);
  void ResetData();

  // Returns true if the underlying storage uses float values.
  bool IsFloatData() const {
    return data_type_ == DATA_FLOAT || data_type_ == DATA_MATRIX;
  }

  // Returns position within the underlying storage for the given array index.
  int GetArrayPos(int idx) const;

  // Returns the size of the underlying storage in bytes.
  int GetStorageBytesSize() const {
    return array_size_ * element_bytes_size_;
  }

  template<typename T> void StoreBoolElementTo(T* dst, int idx) const;
  template<typename T> void SetBoolElementFrom(int idx, const T* src);

  GLenum type_;
  bool is_array_;
  GLint array_size_;
  DataType data_type_;
  GLint components_;
  int element_bytes_size_;
  char* value_;
};

#endif  // GRAPHICS_TRANSLATION_GLES_UNIFORM_VALUE_H_
