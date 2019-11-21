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

#include <stdint.h>

namespace android {
namespace base {

// A few convenience network-related functions. These are mostly provided
// because Wine provides inet_pton(), inet_ntop() and if_nametoindex()
// that raise an "unimplemented function" at runtime :-/

// Expected a dotted-decimal IPv4 address representation in |src|. On success,
// return true and stores the address as 4 bytes in network order at |dst|.
// On failure, return false.
//
// NOTE: This is equivalent to inet_pton(AF_INET, src, dst), and provided
//       because the latter function crashes Wine at runtime!
bool netStringToIpv4(const char* src, uint8_t* dst);

// Expect an IPv6 address representation in |src|. On success, return true
// and stores the address as 16 bytes at |dst|. On failure return false.
//
// NOTE: This is equivalent to inet_ntop(AF_INET6, src, dst), and provided
//       because the latter function crashes Wine at runtime!
bool netStringToIpv6(const char* src, uint8_t* dst);

// Expect |src| to contain a decimal number or the name of one of the
// system's network interface (e.g. 'lo' on Linux is typically used for
// the loopback address). On success, return an index, or -1 on failure.
//
// NOTE: This is nearly equivalent to if_nametoindex(), except that it returns
//       -1 instead of 0 in case of error. This is provided because the
//       corresponding function crashes Wine at runtime!
int netStringToInterfaceIndex(const char* src);

// Testing support. This allows one to inject a custom network interface
// resolver during unit-testing.
class NetworkInterfaceNameResolver {
public:
    virtual ~NetworkInterfaceNameResolver() {}
    virtual int queryInterfaceName(const char* src) = 0;
};

NetworkInterfaceNameResolver* netSetNetworkInterfaceNameResolverForTesting(
        NetworkInterfaceNameResolver* resolver);

}  // namespace base
}  // namespace android
