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
#ifndef GRAPHICS_TRANSLATION_GLES_GLES_UTILS_H_
#define GRAPHICS_TRANSLATION_GLES_GLES_UTILS_H_

#include <GLES/gl.h>
#include <algorithm>

enum GlesVersion {
  // These match the values specified by EGL_CONTEXT_CLIENT_VERSION.
  kGles11 = 1,
  kGles20 = 2,
  kGlesVersionCount
};

template <typename T>
inline T ClampValue(T value, T min, T max) {
  return std::min(std::max(value, min), max);
}

inline GLfloat X2F(const GLfixed value) {
  return static_cast<GLfloat>(value) / 65536.0f;
}

inline GLfixed F2X(const GLfloat value) {
  return value > +32767.65535f ? +32767 * 65536 + 65535 :
         value < -32768.65535f ? -32768 * 65536 + 65535 :
         static_cast<GLfixed>(value * 65536);
}

inline void Convert(GLboolean* data, GLboolean v) {
  *data = v ? GL_TRUE : GL_FALSE;
}

inline void Convert(GLboolean* data, GLfloat v) {
  *data = v ? GL_TRUE : GL_FALSE;
}

inline void Convert(GLboolean* data, GLint v) {
  *data = v ? GL_TRUE : GL_FALSE;
}

inline void Convert(GLboolean* data, GLuint v) {
  *data = v ? GL_TRUE : GL_FALSE;
}

inline void Convert(GLfloat* data, GLboolean v) {
  *data = v ? GL_TRUE : GL_FALSE;
}

inline void Convert(GLint* data, GLboolean v) {
  *data = v ? GL_TRUE : GL_FALSE;
}

inline void Convert(GLfloat* data, GLint v) {
  *data = static_cast<GLfloat>(v);
}

inline void Convert(GLfloat* data, GLuint v) {
  *data = static_cast<GLfloat>(v);
}

inline void Convert(GLint* data, GLuint v) {
  *data = static_cast<GLint>(v);
}

inline void Convert(GLint* data, GLfloat v) {
  *data = static_cast<GLint>(v);
}

inline void Convert(GLfloat* data, GLfloat v) {
  *data = v;
}

inline void Convert(GLint* data, GLint v) {
  *data = v;
}

template <typename T, typename S, size_t N>
inline void Convert(T* data, const S (&v)[N]) {
  for (size_t i = 0; i < N; ++i) {
    Convert(data + i, v[i]);
  }
}

template <typename T, typename I>
inline void Convert(T* data, I begin, I end) {
  while (begin != end) {
    Convert(data, *begin);
    ++begin;
    ++data;
  }
}

#endif  // GRAPHICS_TRANSLATION_GLES_GLES_UTILS_H_
