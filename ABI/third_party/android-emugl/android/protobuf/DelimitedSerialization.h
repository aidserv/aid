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

#include "google/protobuf/message_lite.h"
#include "google/protobuf/io/zero_copy_stream.h"

namespace android {
namespace protobuf {

//
// This file defines methods for serialization of protobuf messages in the same
// way as Java's writeDelimitedTo()/parseDelimitedFrom().
//

bool writeOneDelimited(const google::protobuf::MessageLite& message,
                       google::protobuf::io::ZeroCopyOutputStream* out);

bool readOneDelimited(google::protobuf::MessageLite* message,
                      google::protobuf::io::ZeroCopyInputStream* in);

}  // namespace android
}  // namespace protobuf
