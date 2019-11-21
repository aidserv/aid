/* Copyright (C) 2017 The Android Open Source Project
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
#include "android/proxy/proxy_common.h"
#include "android/proxy/proxy_errno.h"
#include "android/proxy/proxy_setup.h"
#include "android/utils/debug.h"
#include "android/utils/stralloc.h"
#include "android/emulation/control/http_proxy_agent.h"
#include <stdio.h>
#include <string.h>
#define  D(...)  VERBOSE_PRINT(proxy,__VA_ARGS__)

extern AndroidProxyCB *const gAndroidProxyCB;
extern void proxy_manager_atexit(void);
STRALLOC_DEFINE(UIProxy);

int
android_http_proxy_set( const char*  proxy )
{
    int rt = PROXY_ERR_OK;

    D("android_http_proxy_set %s proxy \n", proxy? "add":"remove");
    // remove proxy
    if (proxy == NULL || proxy[0] == '\0') {
        if (stralloc_cstr(UIProxy) != '\0') {
            stralloc_reset(UIProxy);
        }
        if (gAndroidProxyCB->ProxyUnset) {
            gAndroidProxyCB->ProxyUnset();
        }
        proxy_manager_atexit();
        return rt;
    }
    // set proxy
    rt = android_http_proxy_setup(proxy, VERBOSE_CHECK(proxy));
    if (rt == PROXY_ERR_OK)
    {
        if (stralloc_cstr(UIProxy) != '\0') {
            stralloc_reset(UIProxy);
        }
        stralloc_add_str(UIProxy, proxy);
        if(gAndroidProxyCB->ProxySet) {
            gAndroidProxyCB->ProxySet(stralloc_cstr(UIProxy));
        }
    }
    return rt;
}
