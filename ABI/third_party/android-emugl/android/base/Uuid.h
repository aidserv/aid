// Copyright 2016 The Android Open Source Project
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
#include <type_traits>

#include <stdint.h>

namespace android {
namespace base {

// This class incapsulates a Uuid - universally unique identifier, AKA Guid
// It's implemented with Uuid* functions in Windows and libuuid for Posix.
//
class Uuid {
public:
    // Create a zeroed Uuid
    Uuid();

    // Parse a string into Uuid instance. If the string isn't parsable the
    // object is zero-initialized instead
    explicit Uuid(StringView asString);

    // These two functions generate a new random Uuid. generateFast() may
    // be slightly faster because of the higher possibility of
    // non-global uniquiness.
    // Windows has a faster function and it is used here. Current Posix
    // implementation uses the same uuid_generate() for both
    static Uuid generate();
    static Uuid generateFast();

    // Returns a string representation of the Uuid in the following form:
    //      xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
    std::string toString() const;

    // A string form of the zeroed Uuid
    static const char* const nullUuidStr;

    // Set of comparison operators for the Uuids
    friend bool operator<(const Uuid& a, const Uuid& b) {
        return memcmp(&a.mData, &b.mData, sizeof(Uuid::data_t)) < 0;
    }
    friend bool operator>(const Uuid& a, const Uuid& b) { return b < a; }
    friend bool operator==(const Uuid& a, const Uuid& b) {
        return memcmp(&a.mData, &b.mData, sizeof(Uuid::data_t)) == 0;
    }
    friend bool operator!=(const Uuid& a, const Uuid& b) { return !(a == b); }

private:
    // Helpers for casting to a system-specific Uuid type.
    void* dataPtr();
    const void* dataPtr() const;

    // The Uuid is 16-byte number, but underlying system types are different
    // on different OSes.
    // This type alias is able to store any of those
    using data_t = typename std::aligned_storage<
            16, std::alignment_of<int32_t>::value>::type;

    data_t mData;
};

}  // namespace base
}  // namespace android
