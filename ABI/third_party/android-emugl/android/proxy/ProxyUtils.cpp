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

#include "android/base/StringView.h"
#include "android/base/network/Dns.h"
#include "android/base/network/IpAddress.h"

#include <algorithm>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

namespace android {
namespace proxy {

using android::base::IpAddress;
using android::base::Dns;

ParseResult parseConfigurationString(android::base::StringView str) {
    ParseResult result{};

    const char* pos = str.begin();
    const char* end = str.end();

    // Skip optional http:// prefix.
    static constexpr android::base::StringView kHttpPrefix = "http://";
    if (static_cast<size_t>(end - pos) >= kHttpPrefix.size() &&
        !memcmp(pos, kHttpPrefix.c_str(), kHttpPrefix.size())) {
        pos += kHttpPrefix.size();
    }

    // Sanity check.
    if (pos >= end) {
        result.mError = "Empty proxy configuration string";
        return result;
    }

    // Is there a username:password pair?
    const char* sep = std::find(pos, end, '@');
    if (sep < end) {
        const auto sep2 = std::find(pos, sep, ':');
        if (sep2 == sep) {
            result.mError =
                    "Bad format: missing colon between username "
                    "and password";
            return result;
        }

        result.mUsername = std::string(pos, sep2);
        result.mPassword = std::string(sep2 + 1, sep);

        pos = sep + 1;
    }

    // Extract the proxy name. It can contain colons (e.g. IPv6) if it is
    // wrapped with braces.
    const char* proxy = pos;
    const char* proxyEnd = nullptr;
    if (pos < end && pos[0] == '[') {
        sep = std::find(pos + 1, end, ']');
        if (sep == end) {
            result.mError = "Bad format: missing closing bracket";
            return result;
        }
        proxy = pos + 1;
        proxyEnd = sep;
        sep++;
    } else {
        sep = std::find(pos, end, ':');
        proxyEnd = sep;
    }
    if (sep == end || sep[0] != ':' || sep + 1 == end) {
        result.mError = "Bad format: missing colon between proxy and port";
        return result;
    }

    std::string proxyName(proxy, proxyEnd);
    Dns::AddressList list = Dns::resolveName(proxyName);
    if (list.empty()) {
        result.mError = "Bad format: invalid proxy name";
        return result;
    }
    result.mServerAddress = list[0];

    // Then extract port number.
    errno = 0;
    char* portEnd = nullptr;
    long port = strtol(sep + 1, &portEnd, 10);
    if (errno || portEnd != const_cast<char*>(end)) {
        result.mError = "Bad format: invalid port number (must be decimal)";
        return result;
    }
    if (port < 0 || port > 65535) {
        result.mError =
                "Bad format: invalid port number "
                "(must be in 0..65535 range)";
        return result;
    }
    result.mServerPort = static_cast<int>(port);
    return result;
}

}  // namespace proxy
}  // namespace android
