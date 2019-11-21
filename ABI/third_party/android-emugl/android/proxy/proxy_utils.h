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

#include "android/base/Optional.h"
#include "android/base/network/IpAddress.h"

#include <string>

namespace android {
namespace proxy {

// A structure used to model the result of parsing a proxy configuration
// string. |mServerAddress| is the proxy server's IP address, |mUsername|
// and |mPassword| are optional authentication username/passwords. In case
// of error during parsing, |mError| will contain a user-friendly error
// message describing the error.
struct ParseResult {
    android::base::IpAddress mServerAddress;
    android::base::Optional<std::string> mUsername;
    android::base::Optional<std::string> mPassword;
    std::string mError;
};

// Parse a given proxy configuration string. |str| is a string in one of the
// following formats:
//     <hostname>:<port>
//     <username>:<password>@<hostname>:<port>
//
// Where <hostname> can be a hostname, a dotted decimal IPv4 address, or an
// IPv6 address wrapped in braces (e.g. '{::1}') since the colon is used to
// delimit the hostname and the port already.
//
// NOTE: This doesn't support a <username> or <password> that contain a @ or
//       a colon. :-/
bool parseConfigurationString(StringView str, ParseResult* out);

}  // namespace proxy
}  // namespace android
