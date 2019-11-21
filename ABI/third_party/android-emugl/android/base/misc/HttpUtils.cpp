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

#include "android/base/misc/HttpUtils.h"

#include <string.h>

namespace android {
namespace base {

namespace {

#define ARRAYLEN(x)  (sizeof(x)/sizeof(x[0]))

bool isHttpSpace(int ch) {
    // See Section 2.2 of RFC 2616, in particular this section:
    //
    //    All linear white space, including folding, has the same
    //    semantics as SP. A recipient MAY replace any linear white space
    //    with a single SP before interpreting the field value or
    //    forwarding the message downstream.
    //
    return (ch == 9 || ch == 32);
}

}  // namespace

bool httpIsRequestLine(const char* line, size_t lineLen) {
    // First, check whether the line begins with an HTTP method
    static const char* const kHttpMethods[] = {
        "GET",
        "HEAD",
        "POST",
        "PUT",
        "DELETE",
        "TRACE",
        "CONNECT",
    };
    const size_t kHttpMethodsSize = ARRAYLEN(kHttpMethods);

    bool methodMatch = false;

    for (size_t n = 0; n < kHttpMethodsSize; ++n) {
        const char* method = kHttpMethods[n];
        size_t methodLen = strlen(method);

        if (lineLen < methodLen || memcmp(method, line, methodLen)) {
            continue;
        }
        if (isHttpSpace(line[methodLen])) {
            methodMatch = true;
            break;
        }
    }

    if (!methodMatch) {
        return false;
    }

    // Now check that the line ends with HTTP/<version>

    // Get rid of trailing whitespace.
    const char* end = line + lineLen;
    while (end > line && isHttpSpace(end[-1])) {
        end--;
    }

    // Extract HTTP version field.
    const char* version = end;
    while (version > line && !isHttpSpace(version[-1])) {
        version--;
    }

    static const char kVersion[] = "HTTP/";
    const size_t kVersionLen = sizeof(kVersion) - 1U;
    if (version == line || (size_t)(end - version) <= kVersionLen ||
        memcmp(version, kVersion, kVersionLen) != 0) {
        // No HTTP version field.
        return false;
    }

    // Don't check the version field, this look good enough.
    return true;
}

}  // namespace base
}  // namespace android
