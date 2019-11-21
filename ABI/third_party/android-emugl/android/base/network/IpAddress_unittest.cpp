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

#include "android/base/network/IpAddress.h"

#include "android/base/Log.h"
#include "android/base/StringFormat.h"
#include "android/base/testing/GTestUtils.h"
#include "android/base/testing/TestNetworkInterfaceNameResolver.h"

#include <gtest/gtest.h>

#include <string>

namespace android {
namespace base {

TEST(IpAddress, DefaultConstructor) {
    IpAddress addr;
    EXPECT_FALSE(addr.valid());
    EXPECT_FALSE(addr.isIpv4());
    EXPECT_FALSE(addr.isIpv6());
}

TEST(IpAddress, ConstructorIpv4) {
    IpAddress addr1(0x12345678);
    EXPECT_TRUE(addr1.valid());
    EXPECT_TRUE(addr1.isIpv4());
    EXPECT_EQ(0x12345678U, addr1.ipv4());
}

TEST(IpAddress, CopyOperations) {
    IpAddress addr0(0x12345678);

    IpAddress addr1(addr0);  // copy-constructor.
    EXPECT_TRUE(addr1.isIpv4());
    EXPECT_EQ(addr0.ipv4(), addr1.ipv4());
    EXPECT_EQ(addr0.ipv4(), addr0.ipv4());

    IpAddress addr2;
    EXPECT_FALSE(addr2.isIpv4());
    addr2 = addr0;  // copy-assignment
    EXPECT_TRUE(addr2.isIpv4());
    EXPECT_EQ(addr2.ipv4(), addr0.ipv4());
}

TEST(IpAddress, ConstructorIpv6) {
    static const uint8_t addr6[16] = {
            0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0,
            0x0f, 0xed, 0xcb, 0xa9, 0x87, 0x65, 0x43, 0x21,
    };

    IpAddress addr1(addr6);
    EXPECT_TRUE(addr1.valid());
    EXPECT_FALSE(addr1.isIpv4());
    EXPECT_TRUE(addr1.isIpv6());
    EXPECT_TRUE(RangesMatch(addr6, addr1.ipv6Addr()));
}

TEST(IpAddress, ConstructorFromStringIpv4) {
    static const struct {
        const char* input;
        bool expected_valid;
        uint32_t expected_ip;
    } kData[] = {
            {"0.0.0.0", true, 0x00000000U},
            {"127.0.0.1", true, 0x7f000001U},
            {"0.0.0.255", true, 0x000000FFU},
            {"0.0.0.256", false, 0},
            {"1.2.3.45", true, 0x0102032DU},
            {"1.2.3.4", true, 0x01020304U},
            {"1.2.3.", false, 0},
            {"1.2.3", false, 0},
            {"1.2.", false, 0},
            {"1.2", false, 0},
            {"1.", false, 0},
            {"1.", false, 0},
            {"1", false, 0},
    };
    for (const auto& item : kData) {
        std::string text = "Parsing IPv4 address [";
        text += item.input;
        text += "]";
        IpAddress addr(item.input);
        EXPECT_EQ(item.expected_valid, addr.valid()) << text;
        if (item.expected_valid && addr.valid()) {
            EXPECT_TRUE(addr.isIpv4()) << text;
            EXPECT_EQ(item.expected_ip, addr.ipv4()) << text;
        }
    }
}

TEST(IpAddress, ConstructorFromStringIpv6) {
    TestNetworkInterfaceNameResolver resolver;
    resolver.addEntry("loop", 1);
    resolver.addEntry("ether", 2);

    static const struct {
        const char* input;
        bool expected_valid;
        uint8_t expected_ip6[16];
        uint32_t expected_scope_id;
    } kData[] = {
            {"::1", true, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}, 0},
            {"fffe::204.152.189.116",
             true,
             {255, 254, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 204, 152, 189, 116},
             0},
            {"::1%Unknown Interface", false, {}, 0},
            {"::2%15",
             true,
             {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2},
             15},
            {"::3%loop",
             true,
             {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3},
             1},
            {"::4%ether",
             true,
             {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
             2},
    };
    for (const auto& item : kData) {
        std::string text = "Parsing IPv6 address [";
        text += item.input;
        text += "]";
        IpAddress addr(item.input);
        EXPECT_EQ(item.expected_valid, addr.valid()) << text;
        if (item.expected_valid && addr.valid()) {
            EXPECT_TRUE(addr.isIpv6()) << text;
            EXPECT_TRUE(RangesMatch(item.expected_ip6, addr.ipv6Addr()))
                    << text;
            EXPECT_EQ(item.expected_scope_id, addr.ipv6ScopeId()) << text;
        }
    }
}

TEST(IpAddress, toStringIpv4) {
    static const struct {
        uint32_t input;
        StringView expected;
    } kData[] = {
            {0x00000000U, "0.0.0.0"},
            {0x7f000001U, "127.0.0.1"},
            {0xffffffffU, "255.255.255.255"},
    };
    for (const auto& item : kData) {
        IpAddress ip(item.input);
        EXPECT_EQ(std::string(item.expected), ip.toString()) << "For IPv4 "
                                                             << item.input;
    }
}

TEST(IpAddress, toStringIpv6) {
    static const struct {
        uint8_t input[16];
        StringView expected;
    } kData[] = {
            {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, "::"},
            {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}, "::1"},
            {{255, 254, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 204, 152, 189, 116},
             "fffe::cc98:bd74"},
    };
    for (const auto& item : kData) {
        IpAddress ip(item.input);
        std::string text;
        for (int n = 0; n < 16; n += 2) {
            StringAppendFormat(&text, "%02x%02x:", item.input[n],
                               item.input[n + 1]);
        }
        EXPECT_EQ(std::string(item.expected), ip.toString()) << "For IPv6 "
                                                             << text;
    }
}

TEST(IpAddress, hash) {
    static const struct {
        size_t expected;
        const char* address;
    } kData[] = {
            {0x12345678, "<invalid>"},
            {0, "0.0.0.0"},
            {0x7f000001, "127.0.0.1"},
            {1, "::1"},
            {13, "::2%15"},
    };
    for (const auto& item : kData) {
        IpAddress ip(item.address);
        EXPECT_EQ(item.expected, ip.hash()) << "For address [" << item.address
                                            << "]";
    }
}

TEST(IpAddress, operatorEquals) {
    IpAddress ip0;
    IpAddress ip1(0x00000000U);
    IpAddress ip2(0x7f000001U);
    IpAddress ip3("::");
    IpAddress ip4("::1");
    IpAddress ip5("::1%2");
    IpAddress ip6("::1%4");

    EXPECT_TRUE(ip0 == ip0);
    EXPECT_TRUE(ip1 == ip1);
    EXPECT_TRUE(ip2 == ip2);
    EXPECT_TRUE(ip3 == ip3);
    EXPECT_TRUE(ip4 == ip4);
    EXPECT_TRUE(ip5 == ip5);
    EXPECT_TRUE(ip6 == ip6);

    EXPECT_FALSE(ip0 == ip1);
    EXPECT_FALSE(ip0 == ip2);
    EXPECT_FALSE(ip0 == ip3);
    EXPECT_FALSE(ip0 == ip4);
    EXPECT_FALSE(ip0 == ip5);
    EXPECT_FALSE(ip0 == ip6);

    EXPECT_FALSE(ip1 == ip2);
    EXPECT_FALSE(ip1 == ip3);

    EXPECT_FALSE(ip3 == ip4);
    EXPECT_FALSE(ip3 == ip5);
    EXPECT_FALSE(ip3 == ip6);

    EXPECT_FALSE(ip4 == ip5);
    EXPECT_FALSE(ip5 == ip6);
}

TEST(IpAddress, operatorNotEquals) {
    IpAddress ip0;
    IpAddress ip1(0x00000000U);
    IpAddress ip2(0x7f000001U);
    IpAddress ip3("::");
    IpAddress ip4("::1");
    IpAddress ip5("::1%2");
    IpAddress ip6("::1%4");

    EXPECT_FALSE(ip0 != ip0);
    EXPECT_FALSE(ip1 != ip1);
    EXPECT_FALSE(ip2 != ip2);
    EXPECT_FALSE(ip3 != ip3);
    EXPECT_FALSE(ip4 != ip4);
    EXPECT_FALSE(ip5 != ip5);
    EXPECT_FALSE(ip6 != ip6);

    EXPECT_TRUE(ip0 != ip1);
    EXPECT_TRUE(ip0 != ip2);
    EXPECT_TRUE(ip0 != ip3);
    EXPECT_TRUE(ip0 != ip4);
    EXPECT_TRUE(ip0 != ip5);
    EXPECT_TRUE(ip0 != ip6);

    EXPECT_TRUE(ip1 != ip2);
    EXPECT_TRUE(ip1 != ip3);

    EXPECT_TRUE(ip3 != ip4);
    EXPECT_TRUE(ip3 != ip5);
    EXPECT_TRUE(ip3 != ip6);

    EXPECT_TRUE(ip4 != ip5);
    EXPECT_TRUE(ip5 != ip6);
}

TEST(IpAddress, operatorLessThan) {
    IpAddress ip0;
    IpAddress ip1(0x00000000U);
    IpAddress ip2(0x7f000001U);
    IpAddress ip3(0x80808080);
    IpAddress ip4("::");
    IpAddress ip5("::1");
    IpAddress ip6("8080::1");
    IpAddress ip7("::1%2");
    IpAddress ip8("::1%4");

    EXPECT_FALSE(ip0 < ip0);
    EXPECT_TRUE(ip0 < ip1);
    EXPECT_TRUE(ip0 < ip4);

    EXPECT_FALSE(ip1 < ip1);
    EXPECT_TRUE(ip1 < ip2);
    EXPECT_TRUE(ip1 < ip3);
    EXPECT_TRUE(ip2 < ip3);

    EXPECT_FALSE(ip4 < ip4);
    EXPECT_TRUE(ip4 < ip5);
    EXPECT_TRUE(ip5 < ip6);
    EXPECT_TRUE(ip5 < ip7);
    EXPECT_TRUE(ip5 < ip8);
    EXPECT_TRUE(ip7 < ip8);

    EXPECT_TRUE(ip7 < ip6);

    EXPECT_TRUE(ip1 < ip4);
}

}  // namespace base
}  // namespace android
