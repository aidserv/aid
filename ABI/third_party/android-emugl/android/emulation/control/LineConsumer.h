// Copyright 2015 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "android/emulation/control/callbacks.h"

#include "android/base/Compiler.h"

#include <string>
#include <vector>

namespace android {
namespace emulation {

// A canned callback to be used with APIs that require a LineConsumerCallback.
// e.g.:
//   For a function with signature:
//     do_something_fancy(void* opaque, LineConsumerCallback callback);
//   This object can be used as follows:
//     ...
//     LineConsumer lineConsumer;
//     do_something_fancy(lineConsumer.opaque(), LineConsumer::Callback);
//     foreach (const auto& line : lineConsumer.lines()) {
//         // process each line of output.
//     }
//
class LineConsumer {
public:
    LineConsumer() = default;
    // Get an object to be passed as the |opaque| argument for the
    // LineConsumerCallback.
    void* opaque() { return this; }

    // Pass this function as the callback.
    static int Callback(void* opaque, const char* buff, int len);

    // Access the lines consumed by this object.
    const std::vector<std::string>& lines() const { return mLines; }

private:
    std::vector<std::string> mLines;

    DISALLOW_COPY_AND_ASSIGN(LineConsumer);
};

}  // emulation
}  // android
