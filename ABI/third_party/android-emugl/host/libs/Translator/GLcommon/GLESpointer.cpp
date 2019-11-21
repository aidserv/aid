/*
* Copyright (C) 2011 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#include <GLcommon/GLESpointer.h>
#include <stdlib.h>

GLenum GLESpointer::getType() const {
    return m_type;
}

GLint GLESpointer::getSize() const {
    return m_size;
}

GLsizei GLESpointer::getStride() const {
    return m_stride;
}

const GLvoid* GLESpointer::getArrayData() const {
    return m_data;
}

GLvoid* GLESpointer::getBufferData() const {
    return m_buffer
                   ? static_cast<unsigned char*>(m_buffer->getData()) +
                             m_buffOffset
                   : nullptr;
}

const GLfloat* GLESpointer::getValues() const {
    return m_values;
}

unsigned int GLESpointer::getValueCount() const {
    return m_valueCount;
}

const GLvoid* GLESpointer::getData() const {
    switch (m_attribType) {
        case ARRAY:
            return getArrayData();
            break;
        case BUFFER:
            return getBufferData();
            break;
        case VALUE:
            return (GLvoid*)getValues();
            break;
    }
    return nullptr;
}

void GLESpointer::redirectPointerData() {
    m_ownData.resize(0);
    m_data = getBufferData();
}

GLuint GLESpointer::getBufferName() const {
    return m_bufferName;
}

unsigned int GLESpointer::getBufferOffset() const {
    return m_buffOffset;
}

bool GLESpointer::isEnable() const {
    return m_enabled;
}

bool GLESpointer::isNormalize() const {
    return m_normalize;
}

GLESpointer::AttribType GLESpointer::getAttribType() const {
    return m_attribType;
}

bool GLESpointer::isIntPointer() const {
    return m_isInt;
}

void GLESpointer::enable(bool b) {
    m_enabled = b;
}

void GLESpointer::setArray(GLint size,
                           GLenum type,
                           GLsizei stride,
                           const GLvoid* data,
                           GLsizei dataSize,
                           bool normalize,
                           bool isInt) {
    m_ownData.clear();
    m_size = size;
    m_type = type;
    m_stride = stride;
    m_dataSize = dataSize;
    m_data = data;
    m_buffer = nullptr;
    m_bufferName = 0;
    m_buffOffset = 0;
    m_normalize = normalize;
    m_attribType = ARRAY;
    m_isInt = isInt;
}

void GLESpointer::setBuffer(GLint size,
                            GLenum type,
                            GLsizei stride,
                            GLESbuffer* buf,
                            GLuint bufferName,
                            int offset,
                            bool normalize,
                            bool isInt) {
    m_ownData.clear();
    m_size = size;
    m_type = type;
    m_stride = stride;
    m_dataSize = 0;
    m_data = nullptr;
    m_buffer = buf;
    m_bufferName = bufferName;
    m_buffOffset = offset;
    m_normalize = normalize;
    m_attribType = BUFFER;
    m_isInt = isInt;
}

void GLESpointer::setValue(unsigned int count, const GLfloat* val) {
    memcpy(m_values, val, sizeof(GLfloat) * count);
    m_valueCount = count;
    m_attribType = VALUE;
    m_data = nullptr;
    m_buffer = nullptr;
}

void GLESpointer::setDivisor(GLuint divisor) {
    m_divisor = divisor;
}

void GLESpointer::setBindingIndex(GLuint index) {
    m_bindingIndex = index;
}

void GLESpointer::setFormat(GLint size, GLenum type,
                            bool normalize,
                            GLuint reloffset,
                            bool isInt) {
    m_size = size;
    m_type = type;
    m_normalize = normalize;
    m_reloffset = reloffset;
    m_isInt = isInt;
}

void GLESpointer::getBufferConversions(const RangeList& rl, RangeList& rlOut) {
    m_buffer->getConversions(rl, rlOut);
}

GLESpointer::GLESpointer(android::base::Stream* stream) {
    m_size = stream->getBe32();
    m_type = stream->getBe32();
    m_stride = stream->getBe32();
    m_enabled = stream->getByte();
    m_normalize = stream->getByte();
    m_attribType = (GLESpointer::AttribType)stream->getByte();
    m_bufferName = stream->getBe32();
    if (!m_bufferName) {
        m_dataSize = stream->getBe32();
        m_ownData.resize(m_dataSize);
        stream->read(m_ownData.data(), m_dataSize);
        m_data = m_ownData.data();
    }
    m_buffOffset = stream->getBe32();
    m_isInt = stream->getByte();
    m_divisor = stream->getBe32();
    m_bindingIndex = stream->getBe32();
    m_reloffset = stream->getBe32();
    m_valueCount = stream->getBe32();
    stream->read(m_values, sizeof(GLfloat) * m_valueCount);
}

void GLESpointer::onSave(android::base::Stream* stream) const {
    stream->putBe32(m_size);
    stream->putBe32(m_type);
    stream->putBe32(m_stride);
    stream->putByte(m_enabled);
    stream->putByte(m_normalize);
    stream->putByte(m_attribType);
    stream->putBe32(m_bufferName);
    if (!m_bufferName) {
        stream->putBe32(m_dataSize);
        stream->write(m_data, m_dataSize);
    }
    stream->putBe32(m_buffOffset);
    stream->putByte(m_isInt);
    stream->putBe32(m_divisor);
    stream->putBe32(m_bindingIndex);
    stream->putBe32(m_reloffset);
    stream->putBe32(m_valueCount);
    stream->write(m_values, sizeof(GLfloat) * m_valueCount);
}
