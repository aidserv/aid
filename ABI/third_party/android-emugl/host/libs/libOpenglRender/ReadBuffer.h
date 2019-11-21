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
#pragma once
#include "android/base/files/Stream.h"
#include "OpenglRender/IOStream.h"

namespace emugl {

class ReadBuffer {
public:
    explicit ReadBuffer(size_t bufSize);
    ~ReadBuffer();
    int getData(IOStream *stream, int minSize); // get fresh data from the stream
    unsigned char *buf() { return m_readPtr; } // return the next read location
    size_t validData() { return m_validData; } // return the amount of valid data in readptr
    void consume(size_t amount); // notify that 'amount' data has been consumed;

    void onLoad(android::base::Stream* stream);
    void onSave(android::base::Stream* stream);

private:
    unsigned char *m_buf;
    unsigned char *m_readPtr;
    size_t m_size;
    size_t m_validData;
};

}  // namespace emugl
