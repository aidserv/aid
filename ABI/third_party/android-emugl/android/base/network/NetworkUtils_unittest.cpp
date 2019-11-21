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

#include "android/base/network/NetworkUtils.h"
#include "android/base/testing/GTestUtils.h"
#include "android/base/testing/TestNetworkInterfaceNameResolver.h"
#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace android {
namespace base {

TEST(NetworkUtils, netStringToIpv4) {
    static const struct {
        bool expected_result;
        uint8_t expected_ip[4];
        const char* string;
    } kData[] = {
            {false, {}, "0.0.0.0."},
            {false, {}, "0.0.0."},
            {false, {}, ".0.0.0.0"},
            {false, {}, ".0.0.0"},
            {true, {0, 0, 0, 0}, "0.0.0.0"},
            {true, {1, 2, 3, 4}, "1.2.3.4"},
            {true, {192, 168, 10, 1}, "192.168.10.1"},
            {true, {255, 255, 255, 255}, "255.255.255.255"},
            {false, {}, "255.255.255.256"},
    };
    for (const auto& item : kData) {
        uint8_t ip[4];
        bool ret = netStringToIpv4(item.string, ip);
        EXPECT_EQ(item.expected_result, ret) << "For [" << item.string << "]";
        if (ret == item.expected_result && ret) {
            EXPECT_TRUE(RangesMatch(item.expected_ip, ip))
                    << "For [" << item.string << "]";
        }
    }
}

TEST(NetworkUtils, netStringToIpv6) {
    static const struct {
        bool expected_result;
        uint8_t expected_ip[16];
        const char* string;
    } kData[] = {
            {false, {}, "0.0.0.0"},
            {true, {}, "::"},
            {true, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}, "::1"},
            {true,
             {255, 254, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 204, 152, 189, 116},
             "fffe::204.152.189.116"},
    };
    for (const auto& item : kData) {
        uint8_t ip[16];
        bool ret = netStringToIpv6(item.string, ip);
        EXPECT_EQ(item.expected_result, ret) << "For [" << item.string << "]";
        if (ret == item.expected_result && ret) {
            EXPECT_TRUE(RangesMatch(item.expected_ip, ip))
                    << "For [" << item.string << "]";
        }
    }
}

TEST(NetworkUtils, netStringToInterfaceName) {
    TestNetworkInterfaceNameResolver resolver;
    resolver.addEntry("Loopyback", 111);
    resolver.addEntry("Ethernut 3", 230);

    static const struct {
        int expected;
        const char* string;
    } kData[] = {
            {-1, ""},           {-1, "1 "},
            {-1, " 1"},         {1, "1"},
            {1000, "1000"},     {-1, "lo"},
            {-1, "Loopback"},   {-1, "Ethernet 3"},
            {111, "Loopyback"}, {230, "Ethernut 3"},
            {-1, "Ethernet"},   {-1, "Ethernet 2"},
            {-1, "Ethernet 4"},
    };
    for (const auto& item : kData) {
        EXPECT_EQ(item.expected, netStringToInterfaceIndex(item.string))
                << "For [" << item.string << "]";
    }
}

}  // namespace base
}  // namespace android
