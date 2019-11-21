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

#include "android/base/network/IpAddress.h"
#include "android/base/Optional.h"
#include "android/base/StringView.h"

#include <string>

namespace android {
namespace proxy {

// A structure used to model the result of parsing a proxy configuration
// string. |mServerAddress| is the proxy server's IP address, |mUsername|
// and |mPassword| are optional authentication username/passwords. In case
// of error during parsing, |mError| will contain a user-friendly error
// message describing the error.
struct ParseResult {
    android::base::Optional<std::string> mError;
    android::base::IpAddress mServerAddress;
    int mServerPort;
    android::base::Optional<std::string> mUsername;
    android::base::Optional<std::string> mPassword;
};

// Parse a given proxy configuration string. |str| is a string in one of the
// following formats:
//     [http://][<username>:<password>@]<hostname>:<port>
//     [http://][<username>:<password>@]\[<hostname>\]:<port>
//
// Which means that <hostname> can be wrapped around brackets, which is useful
// when it contains colons (e.g. IPv6 addresses such as '[::1]'), since the
// colon is used as a separator between the hostname and the port.
//
// The function returns a ParseResult instance, the caller should first check
// its |mError| field, if it is constructed, an error occured, otherwise the
// rest of the structure is valid and can be used.
//
// NOTE: This doesn't support a <username> or <password> that contain a @ or :
ParseResult parseConfigurationString(android::base::StringView str);

}  // namespace proxy
}  // namespace android
