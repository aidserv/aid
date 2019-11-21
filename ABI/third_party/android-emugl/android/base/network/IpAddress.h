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

#include <functional>
#include <string>

#include <stdint.h>

namespace android {
namespace base {

// A simple structure used to encapsulate an IPv4 or IPv6 address.
// Usage is the following:
//
//   1) IPv4 addresses:
//           IpAddress ip(0x7f000001);   // host-order encoding.
//           CHECK(ip.isIpv4());
//           uint32_t address = ip.ipv4();   // read host-order 32-bit address.
//
//   2) IPv6 address:
//           IpAddress ip({0,0,0,0,0,.....,0,1});   // 16-byte IPv6 address.
//           CHECK(ip.isIpv6());
//           const uint8_t* address = ip.ipv6Addr();
//
//   3) IPv6 address with scope id:
//           IpAddress ip({....}, 1);  // 16-byte address + non-0 scope-id.
//           CHECK(ip.isIpv6());
//           const uint8_t* address = ip.ipv6.addr;
//           uint32_t scope_id = ip.ipv6ScopedId();
//
//   4) Initialize from string representation:
//           IpAddress ip1("127.0.0.1");
//           CHECK(ip1.isValid());
//           CHECK(ip1.isIpv4());
//           ASSERT_EQ(0x7f000001, ip1.ipv4());
//
//           IpAddress ip2("::1");
//           CHECK(ip2.isValid());
//           CHECK(ip2.isIpv6());
//           ASSERT_TRUE(RangesMatch(ip2.ipv6.addr, {0,0,0,....,0,1});
//           ASSERT_EQ(0, ip2.ipv6ScopeId());
//
//           IpAddress ip3("::2@3");
//           CHECK(ip3.isValid());
//           CHECK(ip3.isIpv6());
//           ASSERT_TRUE(RangesMatch(ip3.ipv6Addr(), {0,0,0,....,0,2});
//           ASSERT_EQ(3, ip3.ipv6ScopeId());
//
//   5) Convert to string representation:
//           IpAddress ip = ...;
//           std::string address = ip.toString();
//
class IpAddress {
public:
    using Ipv6Address = uint8_t[16];

    // Default constructor.
    IpAddress() = default;

    // Construct an IPv4 address instance.
    // |ip| is a host-order 32-bit IP address (e.g. 0x7f000001 for 127.0.0.1).
    explicit IpAddress(uint32_t ip) : mKind(Kind::Ipv4), mIpv4(ip) {}

    // Construct an IPv6 address instance. |bytes| contains the address
    // and |scope_id| the optional scope id, which is a system-specific
    // network interface index (see if_nametoindex()).
    IpAddress(const Ipv6Address bytes, uint32_t scope_id = 0);

    // Construct an instance from |string| which must be either an IPv4
    // dotted-decimal address, of a column-separated IPv6 address, possibly
    // followed by a '%' and a scope id. Caller should later call valid()
    // to ensure the input |string| was valid.
    explicit IpAddress(const char* string);

    // A variant of the above constructor that takes a const std::string&
    // parameter instead.
    explicit IpAddress(const std::string& str) : IpAddress(str.c_str()) {}

    // Returns true iff this instance is valid.
    bool valid() const { return mKind == Kind::Ipv4 || mKind == Kind::Ipv6; }

    // Returns true iff this instance is IPv4.
    bool isIpv4() const { return mKind == Kind::Ipv4; }

    // Returns true iff this instance is IPv6.
    bool isIpv6() const { return mKind == Kind::Ipv6; }

    // Return IPv4 address as a 32-bit host-order value.
    // Result is only valid iff isIpv4() is true.
    uint32_t ipv4() const { return mIpv4; }

    // Return the 128-bit address of an IPv6 instance.
    // Result is only valid iff isIpv6() is true, and is a pointer
    // to a 16-byte array of bytes.
    const Ipv6Address& ipv6Addr() const { return mIpv6.mAddr; }

    // Return the optional scope-id of an IPv6 instance.
    // Result is only defined iff isIpv6() is true, and will be 0
    // if there is no scope-id, otherwise will be the system-specific
    // index of a local network interface.
    uint32_t ipv6ScopeId() const { return mIpv6.mScopeId; }

    // Copy and Move operations.
    IpAddress(const IpAddress& other) { copyFrom(this, &other); }

    IpAddress& operator=(const IpAddress& other) {
        if (*this != other) {
            copyFrom(this, &other);
        }
        return *this;
    }

    IpAddress(IpAddress&& other) { copyFrom(this, &other); }

    IpAddress& operator=(IpAddress&& other) {
        copyFrom(this, &other);
        return *this;
    }

    // Equality comparator.
    bool operator==(const IpAddress& other) const;

    bool operator!=(const IpAddress& other) const { return !(*this == other); }

    // Less-than operator, used for inclusion in ordered containers.
    bool operator<(const IpAddress& other) const;

    // Convert instance to a std::string.
    std::string toString() const;

    // Compute a hash for this instance.
    size_t hash() const;

private:
    // Type of supported IP addresses.
    enum class Kind {
        Invalid = -1,
        Ipv4 = 4,
        Ipv6 = 6,
    };

    static void copyFrom(IpAddress* dst, const IpAddress* src);

    Kind mKind = Kind::Invalid;
    union {
        uint32_t mIpv4;  // 32-bit IP address, host-order.
        struct {
            Ipv6Address mAddr;  // 128-bit IPv6 address.
            uint32_t
                    mScopeId;  // 0, or system-specific network interface index.
        } mIpv6;
    };
};

}  // namespace base
}  // namespace android

// std::hash<> support for inclusion in unordered containers.
namespace std {

template <>
struct hash<android::base::IpAddress> {
    typedef android::base::IpAddress argument_type;
    typedef size_t result_type;
    inline result_type operator()(const argument_type& a) const {
        return a.hash();
    }
};

}  // namespace std
