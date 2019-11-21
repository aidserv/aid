// Copyright (C) 2015 The Android Open Source Project
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

#include "android/base/misc/StringUtils.h"

#include <algorithm>

#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
const void* memmem(const void* haystack, size_t haystackLen,
                   const void* needle, size_t needleLen) {
    if (!haystack || !needle) {
        return nullptr;
    }

    const auto it = std::search(
                        static_cast<const char*>(haystack),
                        static_cast<const char*>(haystack) + haystackLen,
                        static_cast<const char*>(needle),
                        static_cast<const char*>(needle) + needleLen);
    return it == static_cast<const char*>(haystack) + haystackLen
            ? nullptr
            : it;
}
#endif  // _WIN32

namespace android {
namespace base {

char* strDup(StringView view) {
    // Same as strdup(str.c_str()) but avoids a strlen() call.
    char* ret = static_cast<char*>(malloc(view.size() + 1u));
    ::memcpy(ret, view.c_str(), view.size());
    ret[view.size()] = '\0';
    return ret;
}

bool strContains(StringView haystack, const char* needle) {
    return ::memmem(haystack.c_str(), haystack.size(),
                    needle, ::strlen(needle)) != nullptr;
}

std::string trim(const std::string& in) {
    size_t start = 0;
    while (start < in.size() && isspace(in[start])) {
        start++;
    }

    size_t end = in.size();
    while (end > start && isspace(in[end - 1])) {
        end--;
    }
    return std::string(in.c_str() + start, end - start);
}

}  // namespace base
}  // namespace android
