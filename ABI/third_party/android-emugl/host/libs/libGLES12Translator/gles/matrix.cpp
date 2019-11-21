// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "matrix.h"

#include <string.h>

#include "common/alog.h"
#include "vector.h"

namespace emugl {

Matrix::Matrix(float row0col0, float row0col1, float row0col2, float row0col3,
               float row1col0, float row1col1, float row1col2, float row1col3,
               float row2col0, float row2col1, float row2col2, float row2col3,
               float row3col0, float row3col1, float row3col2, float row3col3) {
  entries_[0][0] = row0col0;
  entries_[1][0] = row0col1;
  entries_[2][0] = row0col2;
  entries_[3][0] = row0col3;
  entries_[0][1] = row1col0;
  entries_[1][1] = row1col1;
  entries_[2][1] = row1col2;
  entries_[3][1] = row1col3;
  entries_[0][2] = row2col0;
  entries_[1][2] = row2col1;
  entries_[2][2] = row2col2;
  entries_[3][2] = row2col3;
  entries_[0][3] = row3col0;
  entries_[1][3] = row3col1;
  entries_[2][3] = row3col2;
  entries_[3][3] = row3col3;
}

void Matrix::AssignIdentity() {
  entries_[0][0] = 1;
  entries_[0][1] = 0;
  entries_[0][2] = 0;
  entries_[0][3] = 0;
  entries_[1][0] = 0;
  entries_[1][1] = 1;
  entries_[1][2] = 0;
  entries_[1][3] = 0;
  entries_[2][0] = 0;
  entries_[2][1] = 0;
  entries_[2][2] = 1;
  entries_[2][3] = 0;
  entries_[3][0] = 0;
  entries_[3][1] = 0;
  entries_[3][2] = 0;
  entries_[3][3] = 1;
}

void Matrix::Transpose() {
  for (int i = 0; i < kN; ++i) {
    for (int j = i + 1; j < kN; ++j) {
      const float temp = Get(i, j);
      Set(i, j, Get(j, i));
      Set(j, i, temp);
    }
  }
}

void Matrix::RescaleNormal() {
  for (int i = 0; i < 3; ++i) {
    Vector v(Get(i, 0), Get(i, 1), Get(i, 2), 0.f);
    v.Normalize();
    Set(i, 0, v.Get(0));
    Set(i, 1, v.Get(1));
    Set(i, 2, v.Get(2));
  }
}

void Matrix::Inverse() {
  const float a00 = entries_[0][0];
  const float a01 = entries_[0][1];
  const float a02 = entries_[0][2];
  const float a03 = entries_[0][3];
  const float a10 = entries_[1][0];
  const float a11 = entries_[1][1];
  const float a12 = entries_[1][2];
  const float a13 = entries_[1][3];
  const float a20 = entries_[2][0];
  const float a21 = entries_[2][1];
  const float a22 = entries_[2][2];
  const float a23 = entries_[2][3];
  const float a30 = entries_[3][0];
  const float a31 = entries_[3][1];
  const float a32 = entries_[3][2];
  const float a33 = entries_[3][3];

  float b00 = a00 * a11 - a01 * a10;
  float b01 = a00 * a12 - a02 * a10;
  float b02 = a00 * a13 - a03 * a10;
  float b03 = a01 * a12 - a02 * a11;
  float b04 = a01 * a13 - a03 * a11;
  float b05 = a02 * a13 - a03 * a12;
  float b06 = a20 * a31 - a21 * a30;
  float b07 = a20 * a32 - a22 * a30;
  float b08 = a20 * a33 - a23 * a30;
  float b09 = a21 * a32 - a22 * a31;
  float b10 = a21 * a33 - a23 * a31;
  float b11 = a22 * a33 - a23 * a32;

  // Calculate the determinant
  const float det =
    b00 * b11 - b01 * b10 + b02 * b09 + b03 * b08 - b04 * b07 + b05 * b06;
  LOG_ALWAYS_FATAL_IF(det == 0);

  const float invdet = 1.0 / det;
  b00 *= invdet;
  b01 *= invdet;
  b02 *= invdet;
  b03 *= invdet;
  b04 *= invdet;
  b05 *= invdet;
  b06 *= invdet;
  b07 *= invdet;
  b08 *= invdet;
  b09 *= invdet;
  b10 *= invdet;
  b11 *= invdet;

  entries_[0][0] = a11 * b11 - a12 * b10 + a13 * b09;
  entries_[0][1] = a02 * b10 - a01 * b11 - a03 * b09;
  entries_[0][2] = a31 * b05 - a32 * b04 + a33 * b03;
  entries_[0][3] = a22 * b04 - a21 * b05 - a23 * b03;
  entries_[1][0] = a12 * b08 - a10 * b11 - a13 * b07;
  entries_[1][1] = a00 * b11 - a02 * b08 + a03 * b07;
  entries_[1][2] = a32 * b02 - a30 * b05 - a33 * b01;
  entries_[1][3] = a20 * b05 - a22 * b02 + a23 * b01;
  entries_[2][0] = a10 * b10 - a11 * b08 + a13 * b06;
  entries_[2][1] = a01 * b08 - a00 * b10 - a03 * b06;
  entries_[2][2] = a30 * b04 - a31 * b02 + a33 * b00;
  entries_[2][3] = a21 * b02 - a20 * b04 - a23 * b00;
  entries_[3][0] = a11 * b07 - a10 * b09 - a12 * b06;
  entries_[3][1] = a00 * b09 - a01 * b07 + a02 * b06;
  entries_[3][2] = a31 * b01 - a30 * b03 - a32 * b00;
  entries_[3][3] = a20 * b03 - a21 * b01 + a22 * b00;
}

void Matrix::AssignMatrixMultiply(const Matrix& a, const Matrix& b) {
  // If a or b is this matrix, we cannot store result in place, so have to use
  // an extra matrix object.
  const bool use_storage = (this == &a || this == &b);
  Matrix storage(UNINITIALIZED_CONSTRUCTOR);
  Matrix* result = use_storage ? &storage : this;

  for (int a_row = 0; a_row < kN; ++a_row) {
    for (int b_col = 0; b_col < kN; ++b_col) {
      float dp = 0.0f;
      for (int k = 0; k < kN; ++k) {
        dp += a.Get(a_row, k) * b.Get(k, b_col);
      }
      result->Set(a_row, b_col, dp);
    }
  }

  if (use_storage) {
    *this = storage;
  }
}

float* Matrix::GetColumnMajorArray(float* entries, size_t count) const {
  // It's safe to memcpy because entries are stored in column-major order.
  memcpy(entries, entries_, count * sizeof(entries[0]));
  return entries;
}

void Matrix::SetColumnMajorArray(const float *entries) {
  memcpy(entries_, entries, sizeof(entries[0]) * kEntries);
}

Matrix Matrix::GeneratePerspective(float left, float right,
                                   float bottom, float top,
                                   float z_near, float z_far) {
  LOG_ALWAYS_FATAL_IF(left == right);
  LOG_ALWAYS_FATAL_IF(top == bottom);
  LOG_ALWAYS_FATAL_IF(z_near == z_far);

  // See http://www.songho.ca/opengl/gl_projectionmatrix.html.
  return Matrix((2.0f * z_near) / (right - left),
                0.0f,
                (right + left) / (right - left),
                0.0f,

                0.0f,
                (2.0f * z_near) / (top - bottom),
                (top + bottom) / (top - bottom),
                0.0f,

                0.0f,
                0.0f,
                -(z_far + z_near) / (z_far - z_near),
                (-2.0f * z_far * z_near) / (z_far - z_near),

                0.0f,
                0.0f,
                -1.0f,
                0.0f);
}

Matrix Matrix::GenerateOrthographic(float left, float right,
                                    float bottom, float top,
                                    float z_near, float z_far) {
  LOG_ALWAYS_FATAL_IF(left == right);
  LOG_ALWAYS_FATAL_IF(top == bottom);
  LOG_ALWAYS_FATAL_IF(z_near == z_far);

  // See http://www.songho.ca/opengl/gl_projectionmatrix.html.
  return Matrix(2.0f / (right - left),
                0.0f,
                0.0f,
                -(right + left) / (right - left),
                0.0f,
                2.0f / (top - bottom),
                0.0f,
                -(top + bottom) / (top - bottom),
                0.0f,
                0.0f,
                -2.0f / (z_far - z_near),
                -(z_far + z_near) / (z_far - z_near),
                0.0f,
                0.0f,
                0.0f,
                1.0f);
}

Matrix Matrix::GenerateScale(const Vector& v) {
  Matrix result;
  for (int i = 0; i < kN; ++i) {
    result.Set(i, i, v.Get(i));
  }
  return result;
}

Matrix Matrix::GenerateTranslation(const Vector& v) {
  Matrix result;
  for (int i = 0; i < kN; ++i) {
    result.Set(i, 3, v.Get(i));
  }
  return result;
}

Matrix Matrix::GenerateRotationByDegrees(float degrees,
                                         const Vector& v) {
  Vector w = v;
  w.Normalize();
  // See http://mathworld.wolfram.com/RodriguesRotationFormula.html or
  // http://www.manpagez.com/man/3/glRotatef/ for formulas.
  const float theta = degrees * kRadiansPerDegree;
  const float sin_t = sin(theta);
  const float cos_t = cos(theta);
  const float x_cos_t = 1.0f - cos_t;
  const float wx = w.Get(0);
  const float wy = w.Get(1);
  const float wz = w.Get(2);

  return Matrix(cos_t + wx * wx * x_cos_t,
                wx * wy * x_cos_t - wz * sin_t,
                wy * sin_t + wx * wz * x_cos_t,
                0.0f,
                wz * sin_t + wx * wy * x_cos_t,
                cos_t + wy * wy * x_cos_t,
                -wx * sin_t + wy * wz * x_cos_t,
                0.0f,
                -wy * sin_t + wx * wz * x_cos_t,
                wx * sin_t + wy * wz * x_cos_t,
                cos_t + wz * wz * x_cos_t,
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                1.0f);
}

}  // namespace emugl
