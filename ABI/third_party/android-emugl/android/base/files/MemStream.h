// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include "android/base/Compiler.h"
#include "android/base/files/Stream.h"

#include <vector>

namespace android {
namespace base {

// An implementation of the Stream interface on top of a vector.
class MemStream : public Stream {
public:
    using Buffer = std::vector<char>;

    MemStream(int reserveSize = 512);
    MemStream(Buffer&& data);

    int writtenSize() const;
    int readPos() const;
    int readSize() const;

    // Stream interface implementation.
    ssize_t read(void* buffer, size_t size) override;
    ssize_t write(const void* buffer, size_t size) override;

    // Snapshot support.
    void save(Stream* stream) const;
    void load(Stream* stream);

private:
    DISALLOW_COPY_AND_ASSIGN(MemStream);

    Buffer mData;
    int mReadPos = 0;
};

}  // namespace base
}  // namespace android
