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
#pragma once

#include "android/utils/compiler.h"

#include <stdbool.h>

ANDROID_BEGIN_HEADER

// Setup the optional HTTP proxy if necessary. |http_proxy| is either nullptr
// or a string describing the proxy using one of the following formats:
//
//     [http://][<username>:<password>@]<hostname>:<port>
//     [http://][<username>:<password>@]{<hostname>}:<port>
//
// Where <hostname> can be a hostname, a decimal dotted IPv4 IP address,
// or an IPv6 address wrapped around braces (e.g. '{::1}'), since the colon
// is already used as a separated between the hostname and the port.
//
// Return true on success, false otherwise (e.g. if the proxy address is
// invalid, of it connecting to the proxy is impossible, or timed-out after
// a small delay).
int android_http_proxy_setup(const char* http_proxy, bool verbose);

ANDROID_END_HEADER
