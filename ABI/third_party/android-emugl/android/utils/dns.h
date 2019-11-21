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

#pragma once

#include "android/utils/compiler.h"
#include "android/utils/sockets.h"

#include <stddef.h>
#include <stdint.h>

ANDROID_BEGIN_HEADER

// Maximum number of DNS servers supported by the Android emulator.
#define ANDROID_MAX_DNS_SERVERS 4

// List of values returned by android_dns_get_servers() to indicate an
// error:
//
// kAndroidDnsErrorBadServer
//    Either that the server list is malformed
//    or that one of the server names could not be resolved.
//
// kAndroidDnsErrorTooManyServers
//    There are too many servers in the list.
//
enum {
    kAndroidDnsErrorBadServer = -1,
    kAndroidDnsErrorTooManyServers = -2,
};

// Retrieve the IP addresses of DNS servers to be used during emulation.
// |dnsServersOption| is an optional string that corresponds to the -dns-server
// command-line option. If not empty, this must be a comma-separated list of
// DNS server names or addresses. If the list is invalid or the parameter
// NULL or empty, the list of system DNS servers will be probed instead.
// |dnsServerIps| is an array of ANDROID_MAX_DNS_SERVERS 32-bit IP addresses
// that will be filled by the function on success. Return the number of
// IP addresses filled in the array, or a negative kAndroidDnsErrorXXX value
// on error. A return value of 0 is possible and means no DNS server was found.
// A negative value corresponds to a severe error which should stop emulation
// (the function will have printed a message to the console to explain why
// before returning).
int android_dns_get_servers(const char* dnsServersOption,
                            SockAddress* dnsServerIps);

// TODO: Remove the declarations below once QEMU2 uses
//       android_dns_get_servers().

/* Retrieve DNS servers configured by the host system and place them in a
 * provided |buffer|. Up to |bufferSize| DNS servers are retrieved. On success
 * the number of servers is returned, on failure a negative value is returned.
 */
int android_dns_get_system_servers(SockAddress* buffer, size_t bufferSize);

/* Parse a string containing a list of DNS servers separated by comma and place
 * the servers into the provided |buffer|. Up to |bufferSize| servers are
 * parsed. Returns the number of servers added to |buffer| or a negative value
 * on error. Currently the following negative return values have special
 * meaning:
 * -2 Number of servers in |servers| string exceeds |bufferSize|
 */
int android_dns_parse_servers(const char* servers,
                              SockAddress* buffer,
                              size_t bufferSize);

ANDROID_END_HEADER
