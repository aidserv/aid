// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/files/StreamSerializing.h"

namespace android {
namespace base {

void saveStream(Stream* stream, const MemStream& memStream) {
    memStream.save(stream);
}

void loadStream(Stream* stream, MemStream* memStream) {
    memStream->load(stream);
}

}  // namespace base
}  // namespace android
