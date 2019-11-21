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

#include "android/base/network/Dns.h"

#include "android/base/network/IpAddress.h"

#include <string>
#include <vector>
#include <unordered_map>

#include <errno.h>

namespace android {
namespace base {

// Convenience class used to mock the system DNS resolver during unit-testing.
//
// Usage:
//     1) Create TestDnsResolver instance.
//
//     2) Call addEntry(), addEntryIpv4() or addEntryIpv6() on it as many times
//        as needed to populate its table. One can add several addresses per
//        server name.
//
//     3) Run your test, calls to Dns::resolveName() will use the
//        TestDnsResolver() content automatically.
//
class TestDnsResolver : public Dns::Resolver {
public:
    using AddressList = Dns::AddressList;

    // Default constructor injects this instance into the process' global
    // state.
    TestDnsResolver() { mPrevResolver = Dns::Resolver::setForTesting(this); }

    // Destructor restores the previous global resolver instance.
    ~TestDnsResolver() { Dns::Resolver::setForTesting(mPrevResolver); }

    // Clear the internal table.
    void reset() { mMap.clear(); }

    // Add new entry to the internal table. |server_name| is a server name,
    // and |address| is a string corresponding to either a dotted IPv4 address
    // or column-separated IPv6 one.
    void addEntry(StringView server_name, StringView address) {
        AddressList list = {IpAddress(address)};
        addEntryList(server_name, list);
    }

    // Add new entry to the internal table. |server_nam| is a server name,
    // and |ipv4| is the host-order 32-bit IPv4 address for it.
    void addEntryIpv4(StringView server_name, uint32_t ipv4) {
        AddressList list = {IpAddress(ipv4)};
        addEntryList(server_name, list);
    }

    // Add a new entry to the internal table. |server_name| is a server name,
    // and |ipv6| is a 26-byte IPv6 address for it.
    void addEntryIpv6(StringView server_name, const uint8_t ipv6[16]) {
        addEntryList(server_name, AddressList({IpAddress(ipv6)}));
    }

    // Add a |list| of IP addresses to the internal table, associated with
    // |server_name|.
    void addEntryList(StringView server_name, const AddressList& list) {
        std::string key(server_name);
        auto it = mMap.find(key);
        if (it == mMap.end()) {
            mMap.insert(std::make_pair(key, list));
            return;
        }
        for (const auto& item : list) {
            it->second.push_back(item);
        }
    }

    // Return the list of IpAddress instances associated with |server_name|
    // through previous addEntryXXX() calls.
    virtual int resolveName(StringView server_name, AddressList* out) override {
        std::string key(server_name);
        // First try to parse it as a numerical IP address.
        IpAddress ip(key);
        if (ip.valid()) {
            out->push_back(std::move(ip));
            return 0;
        }
        auto it = mMap.find(key);
        if (it == mMap.end()) {
            return -ENOENT;
        }
        *out = it->second;
        return 0;
    }

    // Return the list of DNS servers used by the system currently.
    virtual int getSystemServerList(AddressList* out) override {
        out->emplace_back(IpAddress(0x7f000001));
        return 0;
    }

private:
    Dns::Resolver* mPrevResolver = nullptr;
    std::unordered_map<std::string, AddressList> mMap;
};

}  // namespace base
}  // namespace android
