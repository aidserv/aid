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

#ifndef GRAPHICS_TRANSLATION_GLES_DIRTIABLE_H_
#define GRAPHICS_TRANSLATION_GLES_DIRTIABLE_H_

// A simple container class that also tracks when the contained object is
// (potentially) mutated.
template<typename T>
class Dirtiable {
 public:
  Dirtiable() : value_(), dirty_(true) {}
  explicit Dirtiable(const T& init_value) : value_(init_value), dirty_(true) {}

  bool IsDirty() const { return dirty_; }
  void Clean() { dirty_ = false; }

  const T& Get() const { return value_; }
  T& Mutate() { dirty_ = true; return value_; }

 private:
  T value_;
  bool dirty_;

  Dirtiable(const Dirtiable&);
  Dirtiable& operator=(const Dirtiable&);
};

#endif  // GRAPHICS_TRANSLATION_GLES_DIRTIABLE_H_
