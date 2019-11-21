/* Copyright 2016 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/
#include "android/proxy/ProxyUtils.h"

#include "android/base/Log.h"
#include "android/base/Optional.h"
#include "android/base/network/IpAddress.h"
#include "android/base/testing/TestDnsResolver.h"

#include "gtest/gtest.h"

#include <string>

namespace android {
namespace proxy {

using android::base::IpAddress;
using android::base::Optional;
using android::base::TestDnsResolver;

TEST(ProxyUtils, parseConfigurationString) {
    TestDnsResolver resolver;
    resolver.addEntry("proxy.example.com", "10.0.0.42");

    static const uint8_t kIpv6Loopback[] = {0, 0, 0, 0, 0, 0, 0, 0,
                                            0, 0, 0, 0, 0, 0, 0, 1};
    static const struct {
        const char* input;
        struct {
            IpAddress serverAddress;
            int serverPort;
            Optional<std::string> userName;
            Optional<std::string> password;
        } expected;
    } kData[] = {
            {"127.0.0.1:8080", {IpAddress(0x7f000001U), 8080, {}, {}}},
            {"http://127.0.0.1:8080", {IpAddress(0x7f000001U), 8080, {}, {}}},
            {"user:pass@192.168.0.18:3128",
             {IpAddress(0xc0a80012U), 3128, Optional<std::string>("user"),
              Optional<std::string>("pass")}},
            {"[::1]:7000", {IpAddress(kIpv6Loopback), 7000, {}, {}}},
            {"proxy.example.com:3000", IpAddress(0x0a00002a), 3000, {}, {}},
    };
    for (const auto& item : kData) {
        ParseResult result = parseConfigurationString(item.input);
        EXPECT_FALSE(result.mError);
        if (result.mError) {
            LOG(ERROR) << "Unexpected error [" << *result.mError
                       << "] for input [" << item.input << "]";
            continue;
        }
        EXPECT_EQ(item.expected.serverAddress, result.mServerAddress)
                << "For input [" << item.input << "]";
        EXPECT_EQ(item.expected.serverPort, result.mServerPort)
                << "For input [" << item.input << "]";
        EXPECT_EQ(item.expected.userName, result.mUsername)
                << "For input [" << item.input << "]";
        EXPECT_EQ(item.expected.password, result.mPassword)
                << "For input [" << item.input << "]";
    }
}

TEST(ProxyUtils, parseConfigurationStringWithErrors) {
    TestDnsResolver resolver;

    static const struct {
        const char* input;
        const char* expectedError;
    } kData[] = {
            {"http://", "Empty proxy configuration string"},
            {"", "Empty proxy configuration string"},
            {nullptr, "Empty proxy configuration string"},
            {"256.0.0.1:8080", "Bad format: invalid proxy name"},
            {"127.0.0.1:foo",
             "Bad format: invalid port number (must be decimal)"},
            {"127.0.0.1:-2",
             "Bad format: invalid port number (must be in 0..65535 range)"},
            {"127.0.0.1:100000",
             "Bad format: invalid port number (must be in 0..65535 range)"},
            {"127.0.0.1", "Bad format: missing colon between proxy and port"},
            {"http:127.0.0.1:8080", "Bad format: invalid proxy name"},
            {"::1:8080", "Bad format: invalid proxy name"},
            {"user@pass:127.0.0.1:8080",
             "Bad format: missing colon between username and password"},
            {"user@127.0.0.1:8080",
             "Bad format: missing colon between username and password"},
            {"proxy.example.com:7000", "Bad format: invalid proxy name"},
            {"[::1}:7000", "Bad format: missing closing bracket"},
    };
    for (const auto& item : kData) {
        ParseResult result = parseConfigurationString(item.input);
        EXPECT_TRUE(result.mError) << "For input [" << item.input << "]";
        if (!result.mError) {
            continue;
        }
        EXPECT_STREQ(item.expectedError, result.mError->c_str())
                << "For input [" << item.input << "]";
    }
}

}  // namespace proxy
}  // namespace android
