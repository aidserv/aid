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
#include "android/proxy/proxy_setup.h"

#include "android/proxy/ProxyUtils.h"
#include "android/proxy/proxy_common.h"
#include "android/proxy/proxy_errno.h"
#include "android/proxy/proxy_http.h"
#include "android/utils/debug.h"

#include <errno.h>
#include <string.h>

int android_http_proxy_setup(const char* http_proxy, bool verbose) {
    if (!http_proxy) {
        VERBOSE_DPRINT(init, "Not using any http proxy");
        return PROXY_ERR_OK;
    }

    // Parse the configuration string first.
    android::proxy::ParseResult result =
            android::proxy::parseConfigurationString(http_proxy);
    if (result.mError) {
        dwarning("Ignoring invalid http proxy: %s", result.mError->c_str());
        return PROXY_ERR_BAD_FORMAT;
    }

    if (verbose) {
        proxy_set_verbose(1);
    }

    std::string proxy = result.mServerAddress.toString();
    int proxyPort = result.mServerPort;
    VERBOSE_DPRINT(init, "setting up http proxy: proxy=%s port=%d",
                   proxy.c_str(), proxyPort);

    /* Check that we can connect to the proxy in the next second.
     * If not, the proxy setting is probably garbage !!
     */
    if (proxy_check_connection(proxy.c_str(), proxy.size(), proxyPort, 1000) <
        0) {
        dwarning("Could not connect to proxy at %s:%d: %s !", proxy.c_str(),
                 proxyPort, strerror(errno));
        dwarning("Proxy will be ignored !");
        return PROXY_ERR_UNREACHABLE;
    }

    ProxyOption option_tab[2] = {};
    ProxyOption* option = option_tab;

    if (result.mUsername) {
        option->type = PROXY_OPTION_AUTH_USERNAME;
        option->string = result.mUsername->c_str();
        option->string_len = result.mUsername->size();
        option++;
    }
    if (result.mPassword) {
        option->type = PROXY_OPTION_AUTH_PASSWORD;
        option->string = result.mPassword->c_str();
        option->string_len = result.mPassword->size();
        option++;
    }

    if (proxy_http_setup(proxy.c_str(), proxy.size(), proxyPort,
                         option - option_tab, option_tab) < 0) {
        dwarning("Http proxy setup failed for '%s:%d': %s", proxy.c_str(),
                 proxyPort, errno_str);
        dwarning("Proxy will be ignored !");
        return PROXY_ERR_INTERNAL;
    }

    return PROXY_ERR_OK;
}
