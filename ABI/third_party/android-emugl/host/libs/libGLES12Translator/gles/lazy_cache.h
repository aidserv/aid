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
#ifndef GRAPHICS_TRANSLATION_GLES_LAZY_CACHE_H_
#define GRAPHICS_TRANSLATION_GLES_LAZY_CACHE_H_

#include <string.h>

// A simple lazy cache class that tracks whether the cache has been initialized
// yet or not. If the cached value is requested and it has not yet been
// initialized, then F(T&) is used to do so.
template <typename T, void (*F)(T&)>
class LazyCache {
 public:
  LazyCache() : valid_(false) {}

  T& Mutate() {
    valid_ = true;
    return data_;
  }

  const T& Get() const {
    if (!valid_) {
      // Zero out the data because Chrome validates this in some configurations.
      memset(&data_, 0, sizeof(T));
      F(data_);
      valid_ = true;
    }
    return data_;
  }

 private:
  mutable bool valid_;
  mutable T data_;
};

#endif  // GRAPHICS_TRANSLATION_GLES_LAZY_CACHE_H_
