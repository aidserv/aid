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

#include "google/protobuf/stubs/common.h"

namespace android {
namespace protobuf {

void initProtobufLogger() {
// #define PROTOBUF_DEBUG 1
#if !PROTBUF_DEBUG
    // Silence all parse failure messages, because
    // we handle parse failures ourselves, and because
    // it's annoying to see error messages when not
    // connected to the internet.
    // (LogSilencer doesn't work, since it's for non-fatal
    // messages only)
    google::protobuf::SetLogHandler(nullptr);
#endif
}

} // namespace protobuf
} // namespace android
