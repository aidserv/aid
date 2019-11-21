/*
 * Copyright (C) 2011 The Android Open Source Project
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
#ifndef GRAPHICS_TRANSLATION_GLES_BUFFER_DATA_H_
#define GRAPHICS_TRANSLATION_GLES_BUFFER_DATA_H_

#include <GLES/gl.h>

#include "gles/object_data.h"

#include <string.h>

class BufferData : public ObjectData {
 public:
  explicit BufferData(ObjectLocalName name);

  void SetBufferData(GLenum target, GLuint size, const GLvoid* data,
                     GLuint usage);
  void SetBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size,
                        const GLvoid* data);

  GLuint GetSize() const { return size_; }
  GLenum GetUsage() const { return usage_; }
  const unsigned char* GetData() const { return data_; }

 protected:
  ~BufferData();

 private:
  GLuint size_;
  GLenum usage_;
  unsigned char* data_;

  BufferData(const BufferData&);
  BufferData& operator=(const BufferData&);
};

typedef android::sp<BufferData> BufferDataPtr;

#endif  // GRAPHICS_TRANSLATION_GLES_BUFFER_DATA_H_
