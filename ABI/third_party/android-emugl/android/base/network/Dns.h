// Copyright 2016 The Android Open Source Project
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

#pragma once

#include "android/base/Compiler.h"
#include "android/base/StringView.h"
#include "android/base/network/IpAddress.h"

#include <vector>

namespace android {
namespace base {

// A convenience class to wrap the system's DNS resolver.
//
// Usage:
//    Dns::AddressList list = Dns::resolveName("<server-name>");
//    if (list.empty()) { ... error ... }
//
class Dns {
public:
    using AddressList = std::vector<IpAddress>;

    // Resolve name |server_name| into a list of IpAddress instances.
    // Return an empty list on failure.
    static AddressList resolveName(StringView server_name);

    // Return the list of DNS server addresses used by the system.
    // This works by either parsing /etc/resolver.conf on Posix or
    // calling the appropriate Win32 API.
    static AddressList getSystemServerList();

    // Abstract interface to a system resolver.
    // Usage:
    //   1) Use Resolver::get() to retrieve current instance.
    //
    //   2) Use Resolver::setForTesting() to inject custom version during
    //      unit-tests.
    class Resolver {
    public:
        virtual ~Resolver() = default;

        // Parse |server_name| as a DNS server name or IP address and
        // resolves it to a list of IpAddress instances. On success, return
        // 0 and append results to |*out|. On failure, just return a negative
        // errno code.
        virtual int resolveName(StringView server_name, AddressList* out) = 0;

        // Return the list of DNS servers used by the system. On success,
        // return 0 and append results to |*out|. On failure, just return
        // a negative errno code.
        virtual int getSystemServerList(AddressList* out) = 0;

        // Return current global resolver for the process. This can be
        // altered with setForTesting(). Never returns nullptr. Creates
        // a new instance lazily if necessary.
        static Resolver* get();

        // Change the process-global resolver. Return the old one, or
        // nullptr. |resolver| can be nullptr, in which case a future
        // call to get() will create a new system-level resolver
        // (e.g. reparsing /etc/resolv.conf explicitly).
        static Resolver* setForTesting(Resolver* resolver);
    };
};

}  // namespace base
}  // namespace android
