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

#include "android/utils/dns.h"

#include "android/base/Log.h"

#include <gtest/gtest.h>
#include <string>

TEST(android_dns_parse_servers, IgnoreIpv6) {
    const int kMaxAddresses = 3;
    SockAddress addresses[kMaxAddresses] = {};
    const char kList[] = "127.0.0.1,::1,127.0.1.1";
    char tmp[256];
    EXPECT_EQ(3, android_dns_parse_servers(kList, addresses, kMaxAddresses));
    EXPECT_STREQ("127.0.0.1", sock_address_host_string(&addresses[0], tmp, sizeof(tmp)));
    EXPECT_STREQ("::1", sock_address_host_string(&addresses[1], tmp, sizeof(tmp)));
    EXPECT_STREQ("127.0.1.1", sock_address_host_string(&addresses[2], tmp, sizeof(tmp)));
}

TEST(android_dns_get_system_servers, DumpList) {
    const size_t kMaxServers = ANDROID_MAX_DNS_SERVERS;
    SockAddress ips[kMaxServers];
    int count = android_dns_get_system_servers(ips, kMaxServers);
    EXPECT_GT(count, 0);
    EXPECT_LT(count, (int)kMaxServers);
    char tmp[256];
    for (int n = 0; n < count; ++n) {
        LOG(INFO) << "DNS nameserver " << sock_address_host_string(&ips[n], tmp, sizeof(tmp));
    }
}
