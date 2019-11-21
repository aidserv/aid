/* Copyright (C) 2007-2008 The Android Open Source Project
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

#include "android/proxy/proxy_common.h"
#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

// Configure the transparent proxy engine to use a specific HTTP/HTTPS
// proxy. |servername| and |servernamelen| specify the server's URL.
// |serverport| is the port to connect to.
// |num_options| is the number of options, listed in the |options|
// array, which can be used to specify various ProxyOption key/value
// pairs to configure the proxy.
//
// Returns 0 on success, or -1 on failure.
extern int
proxy_http_setup( const char*         servername,
                  int                 servernamelen,
                  int                 serverport,
                  int                 num_options,
                  const ProxyOption*  options );

ANDROID_END_HEADER
