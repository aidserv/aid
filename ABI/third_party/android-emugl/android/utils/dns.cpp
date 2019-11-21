/* Copyright (C) 2015 The Android Open Source Project
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

#include "dns.h"

#include "android/base/network/Dns.h"
#include "android/base/network/IpAddress.h"

#include "android/utils/debug.h"

#include <algorithm>
#include <string>

using android::base::Dns;
using android::base::IpAddress;

static void sock_address_init_from_IPAddress(SockAddress* addr,
                                             const IpAddress& ip) {
    if (ip.isIpv4()) {
        sock_address_init_inet(addr, ip.ipv4(), 0);
    } else {
        sock_address_init_in6(addr, ip.ipv6Addr(), 0);
    }
}

int android_dns_get_system_servers(SockAddress* buffer, size_t bufferSize) {
    Dns::AddressList list = Dns::getSystemServerList();
    if (list.empty()) {
        derror("Failed to retrieve DNS servers list from the system");
        return kAndroidDnsErrorBadServer;
    }

    size_t n = 0;
    for (const auto& addr : list) {
        if (n >= bufferSize) {
            break;
        }
        sock_address_init_from_IPAddress(&buffer[n++], addr);
    }

    if (n == 0) {
        derror("There are no DNS servers used in this system");
        return kAndroidDnsErrorBadServer;
    }

    return static_cast<int>(n);
}

int android_dns_parse_servers(const char* input,
                              SockAddress* buffer,
                              size_t bufferSize) {
    std::string servers(input);

    std::replace(servers.begin(), servers.end(), ',', '\0');

    size_t count = 0;
    for (size_t pos = 0; pos < servers.size();) {
        const char* server = &servers[pos];
        IpAddress addr(server);
        if (!addr.valid()) {
            return kAndroidDnsErrorBadServer;
        }
        if (count >= bufferSize) {
            return kAndroidDnsErrorTooManyServers;
        }
        sock_address_init_from_IPAddress(&buffer[count++], addr);
        pos = servers.find('\0', pos);
        if (pos == std::string::npos) {
            break;
        }
        ++pos; // Skip the actual null terminator
    }
    return count;
}

int android_dns_get_servers(const char* dnsServerOption,
                            SockAddress* dnsServerIps) {
    const int kMaxDnsServers = ANDROID_MAX_DNS_SERVERS;
    int dnsCount = 0;
    if (dnsServerOption && dnsServerOption[0]) {
        dnsCount = android_dns_parse_servers(dnsServerOption, dnsServerIps,
                                             kMaxDnsServers);
        if (dnsCount == kAndroidDnsErrorTooManyServers) {
            derror("Too may DNS servers listed in -dns-server option, a "
                   "maximum of %d values is supported\n",
                   ANDROID_MAX_DNS_SERVERS);
            return kAndroidDnsErrorTooManyServers;
        }
        if (dnsCount < 0) {  // Bad format in the option.
            derror("Malformed or invalid -dns-server parameter: %s",
                   dnsServerOption);
            return kAndroidDnsErrorBadServer;
        }
    }
    if (!dnsCount) {
        dnsCount = android_dns_get_system_servers(dnsServerIps, kMaxDnsServers);
        if (dnsCount < 0) {
            dnsCount = 0;
            dwarning(
                    "Cannot find system DNS servers! Name resolution will "
                    "be disabled.");
        }
    }
    if (VERBOSE_CHECK(init)) {
        dprintn("emulator: Found %d DNS servers:", dnsCount);
        for (int n = 0; n < dnsCount; ++n) {
            char tmp[256];
            dprintn(" %s", sock_address_host_string(&dnsServerIps[n], tmp, sizeof(tmp)));
        }
        dprintn("\n");
    }

    return dnsCount;
}
