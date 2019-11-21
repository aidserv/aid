// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GLES_MATRIX_H_
#define GLES_MATRIX_H_

#include <math.h>
#include <stddef.h>

#ifdef _WIN32 // M_PI not defined in MS Windows
#define M_PI 3.14159265358979323846
#endif

namespace emugl {

class Vector;

static const float kPi = M_PI;
static const float kRadiansPerDegree = kPi / 180.0f;

// 4x4 floating point matrix.
class Matrix {
 public:
  static const int kN = 4;  // N as in NxN matrix.
  static const int kEntries = kN * kN;

  Matrix() {
    AssignIdentity();
  }

  explicit Matrix(const float* entries) {
    SetColumnMajorArray(entries);
  }

  Matrix(float row0col0, float row0col1, float row0col2, float row0col3,
         float row1col0, float row1col1, float row1col2, float row1col3,
         float row2col0, float row2col1, float row2col2, float row2col3,
         float row3col0, float row3col1, float row3col2, float tow3col3);

  void Set(int row, int col, float value) {
    // The entries are stored in column-major order.
    entries_[col][row] = value;
  }

  float Get(int row, int col) const {
    // The entries are stored in column-major order.
    return entries_[col][row];
  }

  void Inverse();

  void Transpose();

  // Normalizes the x-, y-, and z-scale vector components of the matrix.
  void RescaleNormal();

  void AssignIdentity();

  void AssignMatrixMultiply(const Matrix& a, const Matrix& b);

  const Matrix& operator *=(const Matrix& b) {
    AssignMatrixMultiply(*this, b);
    return *this;
  }

  float* GetColumnMajorArray(float (&entries)[kEntries]) const {
    return GetColumnMajorArray(entries, kEntries);
  }

  float* GetColumnMajorArray(float* entries, size_t count) const;

  void SetColumnMajorArray(const float *entries);

  static Matrix GenerateScale(const Vector& v);

  static Matrix GenerateTranslation(const Vector& v);

  static Matrix GenerateRotationByDegrees(float degrees,
                                          const Vector& v);

  static Matrix GeneratePerspective(float left, float right,
                                    float bottom, float top,
                                    float z_near, float z_far);

  static Matrix GenerateOrthographic(float left, float right,
                                     float bottom, float top,
                                     float z_near, float z_far);

 private:
  enum UninitializedConstructor {
    UNINITIALIZED_CONSTRUCTOR
  };

  explicit Matrix(UninitializedConstructor) {
  }

  float entries_[kN][kN];
};

inline Matrix operator* (Matrix a, const Matrix& b) {
  a *= b;
  return a;
}

}  // namespace emugl

#endif  // GLES_MATRIX_H_
