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

#include "android/protobuf/DelimitedSerialization.h"

#include "google/protobuf/io/coded_stream.h"

#include <stdint.h>

namespace android {
namespace protobuf {

bool writeOneDelimited(const google::protobuf::MessageLite& message,
                       google::protobuf::io::ZeroCopyOutputStream* out) {
    // We create a new coded stream for each message. This is fast.
    google::protobuf::io::CodedOutputStream output(out);

    // Write the size.
    const int size = message.ByteSize();
    output.WriteVarint32(size);

    uint8_t* buffer = output.GetDirectBufferForNBytesAndAdvance(size);
    if (buffer) {
        // Optimization: The message fits in one buffer, so use the faster
        // direct-to-array serialization path.
        message.SerializeWithCachedSizesToArray(buffer);
    } else {
        // Slightly-slower path when the message is multiple buffers.
        message.SerializeWithCachedSizes(&output);
        if (output.HadError()) {
            return false;
        }
    }

    return true;
}

bool readOneDelimited(google::protobuf::MessageLite* message,
                      google::protobuf::io::ZeroCopyInputStream* in) {
    // We create a new coded stream for each message. This is fast,
    // and it makes sure the 64MB total size limit is imposed per-message rather
    // than on the whole stream (See the CodedInputStream interface for more
    // info on this limit).
    google::protobuf::io::CodedInputStream input(in);

    // Read the size.
    uint32_t size;
    if (!input.ReadVarint32(&size)) {
        return false;
    }

    // Tell the stream not to read beyond that size.
    const auto limit = input.PushLimit(size);

    // Parse the message.
    if (!message->MergeFromCodedStream(&input)) {
        return false;
    }
    if (!input.ConsumedEntireMessage()) {
        return false;
    }

    // Release the limit.
    input.PopLimit(limit);

    return true;
}

}  // namespace android
}  // namespace protobuf
