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

#include "android/base/Log.h"
#include "android/base/StringFormat.h"
#include "android/base/system/System.h"

#include <limits.h>

#ifdef __APPLE__
#include <sys/socket.h>
#endif

#ifndef _WIN32
#include <net/if.h>
#endif
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include "android/base/memory/LazyInstance.h"
#include "android/base/sockets/Winsock.h"
#include "android/base/system/Win32UnicodeString.h"
#include <iphlpapi.h>
#include <unordered_map>
#include <string>
#endif

// Set to 1 to display debug messages.
#define DEBUG 0

namespace android {
namespace base {

/*      $OpenBSD: inet_pton.c,v 1.8 2010/05/06 15:47:14 claudio Exp $   */
/*      $OpenBSD: inet_ntop.c,v 1.10 2014/05/17 18:16:14 tedu Exp $     */

/* Copyright (c) 1996 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
 * CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#define INADDRSZ 4
#define IN6ADDRSZ 16
#define INT16SZ 2

using u_char = unsigned char;
using u_int = unsigned int;

bool netStringToIpv4(const char* src, uint8_t* dst) {
    static const char digits[] = "0123456789";
    int saw_digit, octets, ch;
    u_char tmp[INADDRSZ], *tp;

    saw_digit = 0;
    octets = 0;
    *(tp = tmp) = 0;
    while ((ch = *src++) != '\0') {
        const char* pch;

        if ((pch = strchr(digits, ch)) != NULL) {
            u_int val = *tp * 10 + (pch - digits);

            if (val > 255)
                return (0);
            if (!saw_digit) {
                if (++octets > 4)
                    return (0);
                saw_digit = 1;
            }
            *tp = val;
        } else if (ch == '.' && saw_digit) {
            if (octets == 4)
                return (0);
            *++tp = 0;
            saw_digit = 0;
        } else
            return (0);
    }
    if (octets < 4)
        return (0);

    memcpy(dst, tmp, INADDRSZ);
    return (1);
}

bool netStringToIpv6(const char* src, uint8_t* dst) {
    static const char xdigits_l[] = "0123456789abcdef",
                      xdigits_u[] = "0123456789ABCDEF";
    u_char tmp[IN6ADDRSZ], *tp, *endp, *colonp;
    const char *xdigits, *curtok;
    int ch, saw_xdigit, count_xdigit;
    u_int val;

    memset((tp = tmp), '\0', IN6ADDRSZ);
    endp = tp + IN6ADDRSZ;
    colonp = NULL;
    /* Leading :: requires some special handling. */
    if (*src == ':')
        if (*++src != ':')
            return (0);
    curtok = src;
    saw_xdigit = count_xdigit = 0;
    val = 0;
    while ((ch = *src++) != '\0') {
        const char* pch;

        if ((pch = strchr((xdigits = xdigits_l), ch)) == NULL)
            pch = strchr((xdigits = xdigits_u), ch);
        if (pch != NULL) {
            if (count_xdigit >= 4)
                return (0);
            val <<= 4;
            val |= (pch - xdigits);
            if (val > 0xffff)
                return (0);
            saw_xdigit = 1;
            count_xdigit++;
            continue;
        }
        if (ch == ':') {
            curtok = src;
            if (!saw_xdigit) {
                if (colonp)
                    return (0);
                colonp = tp;
                continue;
            } else if (*src == '\0') {
                return (0);
            }
            if (tp + INT16SZ > endp)
                return (0);
            *tp++ = (u_char)(val >> 8) & 0xff;
            *tp++ = (u_char)val & 0xff;
            saw_xdigit = 0;
            count_xdigit = 0;
            val = 0;
            continue;
        }
        if (ch == '.' && ((tp + INADDRSZ) <= endp) &&
            netStringToIpv4(curtok, tp) > 0) {
            tp += INADDRSZ;
            saw_xdigit = 0;
            count_xdigit = 0;
            break; /* '\0' was seen by inet_pton4(). */
        }
        return (0);
    }
    if (saw_xdigit) {
        if (tp + INT16SZ > endp)
            return (0);
        *tp++ = (u_char)(val >> 8) & 0xff;
        *tp++ = (u_char)val & 0xff;
    }
    if (colonp != NULL) {
        /*
         * Since some memmove()'s erroneously fail to handle
         * overlapping regions, we'll do the shift by hand.
         */
        const int n = tp - colonp;
        int i;

        if (tp == endp)
            return (0);
        for (i = 1; i <= n; i++) {
            endp[-i] = colonp[n - i];
            colonp[n - i] = 0;
        }
        tp = endp;
    }
    if (tp != endp)
        return (0);
    memcpy(dst, tmp, IN6ADDRSZ);
    return (1);
}

using android::base::System;

namespace {

#ifdef _WIN32

// Technical note:
//
// This uses GetInterfaceInfo() to retrieve the table of network interfaces
// that have IPv4 enabled, even though this feature would normally be used
// for IPv6-enabled ones. For now just assume these are the same interfaces.
//
// TODO: Find the right Win32 API to list the IPv6-enabled interfaces.
//
struct SystemNetworkInterfaceNameResolver
        : public NetworkInterfaceNameResolver {
    // Default constructor.
    SystemNetworkInterfaceNameResolver() { refresh(); }

    // Find the index for an interface named |src|. Return -1 if not found.
    virtual int queryInterfaceName(const char* src) override {
        // Avoid calling GetInterfaceInfo() on each query.
        if ((System::get()->getUnixTimeUs() - mLastTimestamp) >
            kRefreshTimeoutUs) {
            refresh();
        }
        auto it = mMap.find(std::string(src));
        if (it == mMap.end()) {
            return -1;
        }
        return it->second;
    }

private:
    using InterfaceMap = std::unordered_map<std::string, int>;

    // Call GetInterfaceInfo() and refresh the map of interface names.
    void refresh() {
        InterfaceMap map;

        mLastTimestamp = System::get()->getUnixTimeUs();

        std::string buffer;
        ULONG outBufLen = 0;
        DWORD ret = GetInterfaceInfo(NULL, &outBufLen);
        if (ret == ERROR_INSUFFICIENT_BUFFER) {
            buffer.resize(static_cast<size_t>(outBufLen));
            ret = GetInterfaceInfo(
                    reinterpret_cast<IP_INTERFACE_INFO*>(&buffer[0]),
                    &outBufLen);
        }
        if (ret != NO_ERROR) {
            LOG(ERROR) << "Cannot read network interface table";
            return;
        }

        auto info = reinterpret_cast<IP_INTERFACE_INFO*>(&buffer[0]);
        for (int i = 0; i < info->NumAdapters; ++i) {
            std::string name =
                    Win32UnicodeString::convertToUtf8(info->Adapter[i].Name);
#if DEBUG
            LOG(INFO) << StringFormat("Network interface %2d [%s]",
                                      info->Adapter[i].Index, name.c_str());
#endif
            map.insert(std::make_pair(
                    std::move(name), static_cast<int>(info->Adapter[i].Index)));
        }

        mMap.swap(map);
    }

    InterfaceMap mMap;
    System::Duration mLastTimestamp = 0;
    static constexpr System::Duration kRefreshTimeoutUs = 2000000LL;
};

#else  // !_WIN32

class SystemNetworkInterfaceNameResolver : public NetworkInterfaceNameResolver {
public:
    virtual int queryInterfaceName(const char* src) override {
        int ret = if_nametoindex(src);
        return (ret == 0) ? -1 : ret;
    }
};

#endif  // !_WIN32

NetworkInterfaceNameResolver* sResolver = nullptr;

}  // namespace

int netStringToInterfaceIndex(const char* name) {
    // If |name| is a decimal number, don't try to lookup the name.
    if (name[0] >= '0' && name[0] <= '9') {
        char* end = nullptr;
        long val = strtol(name, &end, 10);
        if (*end != '\0' || val < 0 || val > INT_MAX) {
            // Invalid decimal number.
            return -1;
        }
        return static_cast<int>(val);
    }
    // Otherwise, use the current system resolver.
    NetworkInterfaceNameResolver* resolver = sResolver;
    if (!resolver) {
        sResolver = resolver = new SystemNetworkInterfaceNameResolver();
    }
    return resolver->queryInterfaceName(name);
}

NetworkInterfaceNameResolver* netSetNetworkInterfaceNameResolverForTesting(
        NetworkInterfaceNameResolver* resolver) {
    NetworkInterfaceNameResolver* old = sResolver;
    sResolver = resolver;
    return old;
}

}  // namespace base
}  // namepsace android
