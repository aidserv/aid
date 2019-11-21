// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "vector.h"

#include <assert.h>
#include <algorithm>
#include "common/alog.h"
#include "matrix.h"

namespace emugl {

float Vector::GetDotProduct(const Vector& v) const {
  float dp = 0;
  for (size_t i = 0; i < kEntries; ++i)
    dp += entries_[i] * v.entries_[i];
  return dp;
}

void Vector::AssignMatrixMultiply(const Matrix& a, const Vector& b) {
  const Vector r0(a.Get(0, 0), a.Get(0, 1), a.Get(0, 2), a.Get(0, 3));
  const Vector r1(a.Get(1, 0), a.Get(1, 1), a.Get(1, 2), a.Get(1, 3));
  const Vector r2(a.Get(2, 0), a.Get(2, 1), a.Get(2, 2), a.Get(2, 3));
  const Vector r3(a.Get(3, 0), a.Get(3, 1), a.Get(3, 2), a.Get(3, 3));

  const float x = r0.GetDotProduct(b);
  const float y = r1.GetDotProduct(b);
  const float z = r2.GetDotProduct(b);
  const float w = r3.GetDotProduct(b);

  entries_[0] = x;
  entries_[1] = y;
  entries_[2] = z;
  entries_[3] = w;
}

void Vector::AssignLinearMapping(const float* params, size_t count) {
  LOG_ALWAYS_FATAL_IF(count > kEntries);
  for (size_t i = 0; i < count; i++)
    entries_[i] = params[i];
}

void Vector::AssignLinearMapping(const int32_t* params, size_t count) {
  LOG_ALWAYS_FATAL_IF(count > kEntries);
  AssignLinearMappingHelper(params, count);
}

void Vector::AssignLinearMapping(const int16_t* params, size_t count) {
  LOG_ALWAYS_FATAL_IF(count > kEntries);
  AssignLinearMappingHelper(params, count);
}

void Vector::AssignLinearMapping(const int8_t* params, size_t count) {
  LOG_ALWAYS_FATAL_IF(count > kEntries);
  AssignLinearMappingHelper(params, count);
}

void Vector::AssignLinearMapping(const uint16_t* params, size_t count) {
  LOG_ALWAYS_FATAL_IF(count > kEntries);
  AssignLinearMappingHelper(params, count);
}

void Vector::AssignLinearMapping(const uint8_t* params, size_t count) {
  LOG_ALWAYS_FATAL_IF(count > kEntries);
  AssignLinearMappingHelper(params, count);
}

void Vector::GetLinearMapping(float* params, size_t count) const {
  LOG_ALWAYS_FATAL_IF(count > kEntries);
  for (size_t i = 0; i < count; i++)
    params[i] = entries_[i];
}

void Vector::GetLinearMapping(int32_t* params, size_t count) const {
  LOG_ALWAYS_FATAL_IF(count > kEntries);
  GetLinearMappingHelper(params, count);
}

void Vector::Clamp(float minv, float maxv) {
  for (size_t i = 0; i < kEntries; i++)
    entries_[i] = std::min(std::max(entries_[i], minv), maxv);
}

template <typename T>
void Vector::AssignLinearMappingHelper(const T* params, size_t count) {
  // Maps integer values to floating point.
  // If the integer type is signed, the output floating point values will be
  // in the range [-1.0, +1.0]. Otherwise the output floating point values
  // will be in the range [0.0, +1.0].

  // An alias is convenient.
  typedef std::numeric_limits<T> Limits;

  // We only expect integers here.
  LOG_ALWAYS_FATAL_IF(!Limits::is_integer);

  // Note: We use double precision here the limits of a 32 bit integer cannot be
  // accurately represented in a single precision floating point number.

  // Get the range of the integer
  const double mini = static_cast<double>(Limits::min());
  const double maxi = static_cast<double>(Limits::max());
  // Get the range of the floating point output.
  const double minf = Limits::is_signed ? -1.f : 0.f;
  const double maxf = +1.f;

  for (size_t i = 0; i < count; i++) {
    double v = static_cast<double>(params[i]);
    entries_[i] = (v - mini) * (maxf - minf) / (maxi - mini) + minf;
  }
}

template <typename T>
void Vector::GetLinearMappingHelper(T* params, size_t count) const {
  // Maps floating point values to integers.
  // If the integer type is signed, then the floating point values will be
  // assumed to lie in the range [-1.0, +1.0], otherwise they will be assumed
  // to lie in the range [0.0, +1.0]. Numbers outside of this range will be
  // clamped to the boundary. The integers will be mapped such that the minimum
  // floating point value becomes the minimum integer value, and likewise with
  // the maximum value.

  // An alias is convenient.
  typedef std::numeric_limits<T> Limits;

  // We only expect integers here.
  LOG_ALWAYS_FATAL_IF(!Limits::is_integer);

  // Note: We use double precision here the limits of a 32 bit integer cannot be
  // accurately represented in a single precision floating point number.

  // Get the range of the integer
  const double mini = static_cast<double>(Limits::min());
  const double maxi = static_cast<double>(Limits::max());
  // Get the range of the floating point output.
  const double minf = Limits::is_signed ? -1. : 0.;
  const double maxf = +1.;

  for (size_t i = 0; i < count; i++) {
    double v = entries_[i];
    v = (v - minf) * (maxi - mini) / (maxf - minf) + mini;
    v = std::min(std::max(v, mini), maxi);
    params[i] = static_cast<T>(v);
  }
}

}  // namespace emugl
