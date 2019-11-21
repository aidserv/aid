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

#include "android/base/Uuid.h"

#ifdef _WIN32
#include <rpc.h>
#else
#include <uuid/uuid.h>
#endif

namespace android {
namespace base {

const char* const Uuid::nullUuidStr = "00000000-0000-0000-0000-000000000000";

Uuid::Uuid() : mData() {}

const void* Uuid::dataPtr() const {
    return static_cast<const void*>(&mData);
}

void* Uuid::dataPtr() {
    return const_cast<void*>(const_cast<const Uuid*>(this)->dataPtr());
}

#ifdef _WIN32

// Notes:
//   Windows prototypes take all UUID pointers as non-const, that's why
// we have to const_cast everything. Bummer.
//
//   Rpc strings are unsigned chars. Bummer #2.

Uuid::Uuid(StringView asString) {
    static_assert(sizeof(Uuid::data_t) >= sizeof(UUID),
                  "Uuid::data_t is too small on Windows");

    if (::UuidFromStringA(const_cast<unsigned char*>(
                                  reinterpret_cast<const unsigned char*>(
                                          asString.c_str())),
                          static_cast<UUID*>(dataPtr())) != RPC_S_OK) {
        *this = Uuid();
    }
}

Uuid Uuid::generate() {
    Uuid res;
    ::UuidCreate(static_cast<UUID*>(res.dataPtr()));
    return res;
}

Uuid Uuid::generateFast() {
    Uuid res;
    ::UuidCreateSequential(static_cast<UUID*>(res.dataPtr()));
    return res;
}

std::string Uuid::toString() const {
    unsigned char* str = nullptr;
    if (::UuidToString(const_cast<UUID*>(static_cast<const UUID*>(dataPtr())),
                       &str) != RPC_S_OK ||
        !str) {
        return nullUuidStr;
    }

    std::string res = reinterpret_cast<char*>(str);
    RpcStringFree(&str);
    return res;
}

#else  // !_WIN32

// Some Posix headers have uuid_t as a typedef for an array of chars,
// other headers name a struct uuid_t. This template alias helps to unify them
// for a simpler casting from/to Uuid::data_t

template <class T>
using asSystem = typename std::conditional<
        std::is_array<T>::value,
        typename std::decay<T>::type,  // decay an array to a pointer
        T*  // when casting a struct cast to a pointer
        >::type;

Uuid::Uuid(StringView asString) {
    static_assert(sizeof(Uuid::data_t) >= sizeof(uuid_t),
                  "Uuid::data_t is too small on Posix");

    if (uuid_parse(asString.c_str(),
                   static_cast<asSystem<uuid_t>>(dataPtr())) < 0) {
        *this = Uuid();
    }
}

Uuid Uuid::generate() {
    Uuid res;
    uuid_generate(static_cast<asSystem<uuid_t>>(res.dataPtr()));
    return res;
}

Uuid Uuid::generateFast() {
    // for Posix it's the same
    return generate();
}

std::string Uuid::toString() const {
    // allocate enough memory for the uuid string representation
    // including null-terminator
    std::string res(strlen(nullUuidStr) + 1, '\0');
    uuid_unparse_lower(static_cast<asSystem<const uuid_t>>(dataPtr()),
                       &res[0]);
    res.pop_back();  // get rid of the null-terminator now
    return res;
}

#endif  // !_WIN32

}  // namespace base
}  // namespace android
