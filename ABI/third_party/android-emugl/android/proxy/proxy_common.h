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

#include "android/utils/compiler.h"
#include "android/utils/sockets.h"

ANDROID_BEGIN_HEADER

// Common interface function for the transparent proxy engine.
//
// Usage is the following:
//
// - Register the remote proxy server using the appropriate function. For
//   example, use proxy_http_setup() (declared in proxy_http.h) to tell the
//   engine to use a specific HTTP/HTTPS proxy.
//
// - For each new connection to a remote server, call proxy_manager_add()
//   passing the server's address and desired socket type. The function
//   tries to connect to the remote address asynchronously through the
//   configured proxy server, and may return immediately.
//
// - The callback passed to proxy_managed_add() will be called when the
//   connection succeeds or fails. In case of success, a socket file descriptor
//   is also passed, and should be used by the client to communicate with the
//   remote server through the proxy.
//
//   Note that this happens within the thread's current looper's event loop.
//
// - Call proxy_manager_del() to force-fully remove a proxified socket
//   connection. Closing the socket will also work.

/* types and definitions used by all proxy connections */

// Connection attempt result.
typedef enum {
    PROXY_EVENT_NONE,
    PROXY_EVENT_CONNECTED,
    PROXY_EVENT_CONNECTION_REFUSED,
    PROXY_EVENT_SERVER_ERROR
} ProxyEvent;

// This function is called to report the result of an asynchronous
// connection attempt started by proxy_managed_add() below.
// |opaque| is a unique client-provided pointer, that comes from the
// initial proxy_manager_add() call, and can also be used to call
// proxy_manager_del(). |fd| is the socket descriptor to use from the
// client, in case of success. |event| indicates whether the connection
// succeeded or not. It cannot be PROXY_EVENT_NONE.
typedef void (*ProxyEventFunc)( void*  opaque, int  fd, ProxyEvent  event );

// Set |mode| to non-0 value to enable proxy logs.
extern void  proxy_set_verbose(int  mode);

// List of proxy option types.
// PROXY_OPTION_AUTH_USERNAME: value is proxy username.
// PROXY_OPTION_AUTH_PASSWORD: value is proxy password.
// PROXY_OPTION_HTTP_NOCACHE: force HTTP No-Cache option.
// PROXY_OPTION_HTTP_KEEPALIVE:
// PROXY_OPTION_HTTP_USER_AGENT:
// PROXY_OPTION_TYPE: if present, forces the type of proxy to use. Valid
// values are "http" and "https". This is used during unit-testing. Otherwise
// the type is auto-detected based on the current configuration and target
// server address' port (80 for "http", anything else for "https").
typedef enum {
    PROXY_OPTION_AUTH_USERNAME = 1,
    PROXY_OPTION_AUTH_PASSWORD,

    PROXY_OPTION_HTTP_NOCACHE = 100,
    PROXY_OPTION_HTTP_KEEPALIVE,
    PROXY_OPTION_HTTP_USER_AGENT,

    PROXY_OPTION_TYPE,

    PROXY_OPTION_MAX  // Must be last.
} ProxyOptionType;

// Proxy option key/value pair.
typedef struct {
    ProxyOptionType  type;
    const char*      string;
    int              string_len;
} ProxyOption;


// Start a new connection attempt to the target server's |address|.
// |sock_type| determines the type of socket to use. |ev_func| and |ev_opaque|
// specify a callback function and parameter that will be called when the
// connection is established or fails.
//
// Note that the callback can be invoked before this function returns
// (e.g. if the connection immediately succeeds, or if there is a problem
// with the server's address). Otherwise, it will be invoked later during the
// current thread's main loop processing.
//
// Returns 0 on success, or -1 if there is no proxy service for this type of
// connection.
extern int   proxy_manager_add( const SockAddress*   address,
                                SocketType           sock_type,
                                ProxyEventFunc       ev_func,
                                void*                ev_opaque );

// Remove a pending proxified socket connection from the manager's list.
// this is only necessary when the socket connection must be canceled before
// the connection accept/refusal occured. |ev_opaque| must be the same value
// that was passed to proxy_manager_add().
extern void  proxy_manager_del( void*  ev_opaque );

// This function checks that one can connect to a given proxy. It will simply
// try to connect() to it, for a specified timeout, in milliseconds, then close
// the connection.
//
// returns 0 in case of success, and -1 in case of error. errno will be set
// to ETIMEOUT in case of timeout, or ECONNREFUSED if the connection is
// refused.
extern int   proxy_check_connection( const char* proxyname,
                                     int         proxyname_len,
                                     int         proxyport,
                                     int         timeout_ms );

// Returns a string describing the PROXY_ERR_* code
extern const char* proxy_error_string(int errorCode);

ANDROID_END_HEADER
