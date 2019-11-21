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
#include "gles/buffer_data.h"

BufferData::BufferData(ObjectLocalName name)
  : ObjectData(BUFFER, name),
    size_(0),
    usage_(GL_STATIC_DRAW),
    data_(NULL) {
}

BufferData::~BufferData() {
  delete[] data_;
}

void BufferData::SetBufferData(GLenum target, GLuint size, const GLvoid* data,
                               GLuint usage) {
  // TODO(crbug.com/482070): Only keep a copy of the data for element array
  // buffers (ie. target == GL_ELEMENT_ARRAY_BUFFER).  This is because we may
  // need to use this data during glDrawElements calls.  See
  // PointerContext::PrepareElements for more information.
  if (size > size_) {
    delete[] data_;
    data_ = new unsigned char[size];
  }
  size_ = size;
  usage_ = usage;
  if (data_ && data) {
    memcpy(data_, data, size);
  }
}

void BufferData::SetBufferSubData(GLenum target, GLintptr offset,
                                  GLsizeiptr size, const GLvoid* data) {
  if (static_cast<GLuint>(offset) + static_cast<GLuint>(size) > size_) {
    return;
  }
  if (data_ && data) {
    memcpy(data_ + offset, data, size);
  }
}
