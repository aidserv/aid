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

#include "android/base/StringView.h"

#include <string>

#include <inttypes.h>
#include <sys/types.h>

namespace android {
namespace base {

// Abstract interface to byte streams of all kind.
// This is mainly used to implement disk serialization.
class Stream {
public:
    // Default constructor.
    Stream() {}

    // Destructor.
    virtual ~Stream() {}

    // Read up to |size| bytes and copy them to |buffer|. Return the number
    // of bytes that were actually transferred, or -errno value on error.
    virtual ssize_t read(void* buffer, size_t size) = 0;

    // Write up to |size| bytes from |buffer| into the stream. Return the
    // number of bytes that were actually transferred, or -errno value on
    // error.
    virtual ssize_t write(const void* buffer, size_t size) = 0;

    // Write a single byte |value| into the stream. Ignore errors.
    void putByte(uint8_t value);

    // Write a 16-bit |value| as big-endian into the stream. Ignore errors.
    void putBe16(uint16_t value);

    // Write a 32-bit |value| as big-endian into the stream. Ignore errors.
    void putBe32(uint32_t value);

    // Write a 64-bit |value| as big-endian into the stream. Ignore errors.
    void putBe64(uint64_t value);

    // Read a single byte from the stream. Return 0 on error.
    uint8_t getByte();

    // Read a single big-endian 16-bit value from the stream.
    // Return 0 on error.
    uint16_t getBe16();

    // Read a single big-endian 32-bit value from the stream.
    // Return 0 on error.
    uint32_t getBe32();

    // Read a single big-endian 64-bit value from the stream.
    // Return 0 on error.
    uint64_t getBe64();

    // Write a 32-bit float |value| to the stream.
    void putFloat(float value);

    // Read a single 32-bit float value from the stream.
    float getFloat();

    // Write a string |str| into the stream. Ignore errors.
    void putString(StringView str);

    // Write a 0-terminated C string |str| into the stream. Ignore error.
    void putString(const char* str);

    // Write a string |str| of |strlen| bytes into the stream.
    // Ignore errors.
    virtual void putString(const char* str, size_t strlen);

    // Read a string from the stream. Return a new string instance,
    // which will be empty on error. Note that this can only be used
    // to read strings that were written with putString().
    virtual std::string getString();
};

}  // namespace base
}  // namespace android
