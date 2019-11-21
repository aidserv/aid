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

#include <string>
#include <unordered_set>

#ifdef _WIN32
#include "android/base/sockets/Winsock.h"
#include "android/base/sockets/SocketErrors.h"
#include "android/base/system/Win32UnicodeString.h"
#include "android/base/system/Win32Utils.h"
#include "iphlpapi.h"
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#endif

#include <fstream>

#include <errno.h>
#include <string.h>

// Set to 1 to increase verbosity of debug messages.
#define DEBUG 0

namespace android {
namespace base {

namespace {

using AddressList = android::base::Dns::AddressList;

#if DEBUG
#ifdef _WIN32
// Convenience function to save typing.
static std::string toUtf8(const wchar_t* str) {
    return android::base::Win32UnicodeString::convertToUtf8(str);
}
#endif  // !_WIN32
#endif  // _DEBUG

// Implementation of Dns::Resolver interface based on ::getaddrinfo().
class SystemResolver : public Dns::Resolver {
public:
    virtual int resolveName(StringView dns_server_name,
                            AddressList* out) override {
        std::string hostname = dns_server_name;
        struct addrinfo* res;
        struct addrinfo* pHints = nullptr;
#ifdef _WIN32
        struct addrinfo hints = {};
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        pHints = &hints;
#endif
        int ret = ::getaddrinfo(hostname.c_str(), nullptr, pHints, &res);
        if (ret != 0) {
            DLOG(ERROR) << "getaddrinfo('" << dns_server_name << "') returned "
                        << ret;
            switch (ret) {
                case EAI_AGAIN:  // server is down.
                case EAI_FAIL:   // server is sick.
                    return -EHOSTDOWN;

#if defined(EAI_NODATA) && (EAI_NODATA != EAHO_NONAME)
                case EAI_NODATA:
#endif
                case EAI_NONAME:
                    return -ENOENT;

                case EAI_MEMORY:
                    return -ENOMEM;

                default:
                    return -EINVAL;
            }
        }

        // Parse the returned list of addresses.
        for (struct addrinfo* r = res; r != nullptr; r = r->ai_next) {
            // On Linux, getaddrinfo() will return the same address with up to
            // *three* struct addrinfo that only differ by their
            // |ai_socktype| field, which will be SOCK_STREAM, SOCK_DGRAM,
            // and SOCK_RAW. Ignore the ones that are not SOCK_STREAM
            // since we don't expect a machine to have different IP
            // addresses for TCP and UDP.
            if (r->ai_socktype != SOCK_STREAM) {
                continue;
            }
            switch (r->ai_family) {
                case AF_INET: {
                    auto sin =
                            reinterpret_cast<struct sockaddr_in*>(r->ai_addr);
                    out->push_back(IpAddress(ntohl(sin->sin_addr.s_addr)));
                    break;
                }

                case AF_INET6: {
                    auto sin6 =
                            reinterpret_cast<struct sockaddr_in6*>(r->ai_addr);
                    out->push_back(IpAddress(sin6->sin6_addr.s6_addr,
                                             sin6->sin6_scope_id));
                    break;
                }
            }
        }

        ::freeaddrinfo(res);

        return 0;
    }

    virtual int getSystemServerList(AddressList* out) override {
#ifdef _WIN32
        // The set of DNS server addresses already in the output list,
        // used to remove duplicates.
        std::unordered_set<IpAddress> present;

        //Get preferred DNS server IP from GetNetworkParams
        std::string dnsBuffer;
        ULONG outBufLen = sizeof (FIXED_INFO);
        dnsBuffer.resize(static_cast<size_t>(outBufLen));
        DWORD retVal = GetNetworkParams(
            reinterpret_cast<FIXED_INFO*>(&dnsBuffer[0]), &outBufLen);
        if (retVal == ERROR_BUFFER_OVERFLOW) {
            dnsBuffer.resize(static_cast<size_t>(outBufLen));
            retVal = GetNetworkParams(
                       reinterpret_cast<FIXED_INFO*>(&dnsBuffer[0]),
                       &outBufLen);
        }
        if (retVal == ERROR_SUCCESS)
        {
            IP_ADDR_STRING *ipAddr;
            ipAddr =
                &(reinterpret_cast<FIXED_INFO*>(&dnsBuffer[0])->DnsServerList);
            while (ipAddr) {
                IpAddress ip(ipAddr->IpAddress.String);
                if (ip.valid()) {
                    auto ret = present.insert(ip);
                    if (ret.second) {
                        out->emplace_back(std::move(ip));
                    }
                }
                ipAddr = ipAddr->Next;
            };
        }

        std::string buffer;
        ULONG bufferLen = 0;
        DWORD ret = GetAdaptersAddresses(AF_UNSPEC, 0, nullptr, nullptr,
                                         &bufferLen);
        if (ret == ERROR_BUFFER_OVERFLOW) {
            buffer.resize(static_cast<size_t>(bufferLen));
            ret = GetAdaptersAddresses(
                    AF_UNSPEC, 0, nullptr,
                    reinterpret_cast<IP_ADAPTER_ADDRESSES*>(&buffer[0]),
                    &bufferLen);
        }
        if (ret != ERROR_SUCCESS) {
            DLOG(ERROR) << "GetAdaptersAddresses() returned error: "
                        << Win32Utils::getErrorString(ret);
            return -ENOENT;
        }

        // Technical note: Windows DNS resolution with multiple adapters
        // is tricky and detailed in the following document:
        //
        //   https://technet.microsoft.com/en-us/library/dd197552(WS.10).aspx
        //
        // In a nutshell, there is a "preferred network adapter" and its list
        // of DNS servers will be considered first. If there is no response
        // after some time from them, the DNS server list from other adapters
        // will be considered.
        //
        // The tricky part is that there doesn't seem to be a reliable way
        // to determine the preferred network adapter, or the order of
        // alternative ones that are used in case of fallback.
        //
        // Here, the code will simply call GetAdaptersAddresses() to list
        // the DNS server list for all interfaces. It then makes the following
        // assumptions:
        //
        // - The adapters are listed in order of preference, i.e. the DNS
        //   servers from the first adapter should appear in the result
        //   before those found in adapters that appear later in the list.
        //
        // - DNS servers can appear multiple times (i.e. associated with
        //   multiple network adapters), and it's useless to return duplicate
        //   entries in the result.
        //

        for (auto adapter = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(&buffer[0]);
             adapter != nullptr; adapter = adapter->Next) {
#if DEBUG
            LOG(INFO) << "Network Adapter " << adapter->AdapterName;
            LOG(INFO) << "  Friendly name: " << toUtf8(adapter->FriendlyName);
            LOG(INFO) << "  Description  : " << toUtf8(adapter->Description);
            LOG(INFO) << "  DNS suffix   : " << toUtf8(adapter->DnsSuffix);
            LOG(INFO) << "  DNS servers  :";
#endif  // DEBUG
            for (auto dnsServer = adapter->FirstDnsServerAddress;
                 dnsServer != nullptr; dnsServer = dnsServer->Next) {
                auto address = dnsServer->Address.lpSockaddr;
                IpAddress ip;
                switch (address->sa_family) {
                    case AF_INET: {
                        auto sin = reinterpret_cast<sockaddr_in*>(address);
                        ip = IpAddress(ntohl(sin->sin_addr.s_addr));
                        break;
                    }
                    case AF_INET6: {
                        auto sin6 = reinterpret_cast<sockaddr_in6*>(address);
                        ip = IpAddress(sin6->sin6_addr.s6_addr);
                        break;
                    }
                }
#if DEBUG
                LOG(INFO) << "    " << ip.toString();
#endif
                if (ip.valid()) {
                    auto ret = present.insert(ip);
                    if (ret.second) {
                        // A new item was inserted into the set, so save it
                        // into the result.
                        out->emplace_back(std::move(ip));
                    }
                }
            }
        }

        if (present.empty()) {
            return -ENOENT;
        }
        return 0;
#else  // !_WIN32
        std::ifstream fin;
#ifdef __APPLE__
        // on Darwin /etc/resolv.conf is a symlink to
        // /private/var/run/resolv.conf
        // in some situations, the symlink can be destroyed and the system will
        // not
        // re-create it. Darwin-aware applications will continue to run, but
        // "legacy" Unix ones will not.
        fin.open("/private/var/run/resolv.conf");
        if (!fin) {
            fin.open("/etc/resolv.conf");
        }
#else
        fin.open("/etc/resolv.conf");
#endif
        if (!fin.good()) {
            return -ENOENT;
        }

        std::string line;
        size_t count = 0;
        while (std::getline(fin, line)) {
            char nameserver[257];
            if (sscanf(line.c_str(), "nameserver%*[ \t]%256s", nameserver) ==
                1) {
                IpAddress ip(nameserver);
                if (ip.valid()) {
                    out->emplace_back(std::move(ip));
                    count++;
                }
            }
        }

        if (count == 0) {
            return -ENOENT;
        }

        return 0;
#endif  // !_WIN32
    }
};

// Current process-global Resolver instance.
Dns::Resolver* sResolver = nullptr;

}  // namespace

// static
AddressList Dns::resolveName(StringView serverName) {
    AddressList result;
    int err = Dns::Resolver::get()->resolveName(serverName, &result);
    if (err != 0) {
        result.clear();
    }
    return result;
}

// static
AddressList Dns::getSystemServerList() {
    AddressList result;
    int err = Dns::Resolver::get()->getSystemServerList(&result);
    if (err != 0) {
        result.clear();
    }
    return result;
}

// static
Dns::Resolver* Dns::Resolver::get() {
    if (!sResolver) {
        sResolver = new SystemResolver();
    }
    return sResolver;
}

// static
Dns::Resolver* Dns::Resolver::setForTesting(Dns::Resolver* resolver) {
    Dns::Resolver* result = sResolver;
    sResolver = resolver;
    return result;
}

}  // namespace base
}  // namespace android
