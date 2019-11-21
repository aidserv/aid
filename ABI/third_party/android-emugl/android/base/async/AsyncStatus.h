// Copyright 2014 The Android Open Source Project
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

namespace android {
namespace base {

// A list of values indicating the state of ongoing asynchronous operations.
// This is returned by various methods of helper classes like AsyncReader
// or AsyncWriter to indicate to the caller what to do next.
//
// |kAsyncCompleted| means the operation completed properly.
// |kAsyncAgain| means the operation hasn't finished yet.
// |kAsyncError| indicates that an i/o error occured and was detected.
//
enum AsyncStatus {
    kAsyncCompleted = 0,
    kAsyncAgain,
    kAsyncError
};

}  // namespace base
}  // namespace android
