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
#ifndef GRAPHICS_TRANSLATION_GLES_MATRIX_STACK_H_
#define GRAPHICS_TRANSLATION_GLES_MATRIX_STACK_H_

#include <stack>
#include "matrix.h"

// Never-empty stack of matrices.  Initially has an identity matrix in it
// and one entry will always remain.  Its max depth is kMaxDepth.
class MatrixStack {
 public:
  static const size_t kMaxDepth = 32;
  MatrixStack() {
    stack_.push(emugl::Matrix());
  }

  bool Push() {
    if (stack_.size() >= kMaxDepth)
      return false;
    stack_.push(stack_.top());
    return true;
  }

  bool Pop() {
    if (stack_.size() <= 1)
      return false;
    stack_.pop();
    return true;
  }

  const emugl::Matrix& GetTop() const {
    return stack_.top();
  }

  emugl::Matrix& GetTop() {
    return stack_.top();
  }

  int GetDepth() const {
    return stack_.size();
  }

 private:
  std::stack<emugl::Matrix> stack_;

  MatrixStack(const MatrixStack&);
  MatrixStack& operator=(const MatrixStack&);
};

#endif  // GRAPHICS_TRANSLATION_GLES_MATRIX_STACK_H_
