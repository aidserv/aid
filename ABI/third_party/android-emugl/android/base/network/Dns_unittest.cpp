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

#include "android/base/Log.h"
#include "android/base/testing/GTestUtils.h"
#include "android/base/testing/TestDnsResolver.h"

#include <gtest/gtest.h>

namespace android {
namespace base {

TEST(Dns, ResolveLocalhost) {
    // Use the system resolver to resolve 'localhost' and print the result.
    // Don't check anything because the host system could be literally
    // anything :)
    Dns::AddressList list = Dns::resolveName("localhost");
#ifndef _WIN32
    // Windows doesn't know anything about 'localhost', the loopback address
    // can have a sophisticated name like "Loopback Pseudo-Interface 1" but
    // don't count on it (and multiple loopbacks can be installed too in
    // certain cases). So only check that the list is not empty on Posix.
    EXPECT_FALSE(list.empty());
#endif
    for (const auto& address : list) {
        LOG(INFO) << "localhost: " << address.toString();
    }
}

TEST(Dns, ResolveUnknownHost) {
    Dns::AddressList list = Dns::resolveName("does.not.exist.example.com");
    EXPECT_TRUE(list.empty());
}

TEST(Dns, GetSystemServerList) {
    Dns::AddressList list = Dns::getSystemServerList();
    EXPECT_NE(0U, list.size());
    for (const auto& address : list) {
        LOG(INFO) << "DNS nameserver: " << address.toString();
    }
}

TEST(Dns, TestDnsResolver) {
    TestDnsResolver resolver;
    resolver.addEntry("localhost", "127.0.0.2");
    resolver.addEntry("www.example.com", "10.0.3.4");
    resolver.addEntry("www.example.com", "::10.0.3.4");

    Dns::AddressList list = Dns::resolveName("localhost");
    EXPECT_EQ(1U, list.size());
    EXPECT_TRUE(list[0].valid());
    EXPECT_TRUE(list[0].isIpv4());
    EXPECT_EQ(0x7f000002U, list[0].ipv4());

    list = Dns::resolveName("www.example.com");
    EXPECT_EQ(2U, list.size());
    EXPECT_TRUE(list[0].valid());
    EXPECT_TRUE(list[0].isIpv4());
    EXPECT_EQ(0x0a000304U, list[0].ipv4());

    static const uint8_t kExpected[16] = {0, 0, 0, 0, 0,  0, 0, 0,
                                          0, 0, 0, 0, 10, 0, 3, 4};
    EXPECT_TRUE(list[1].isIpv6());
    EXPECT_TRUE(RangesMatch(kExpected, list[1].ipv6Addr()));
}

}  // namespace base
}  // namespace android
