// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include "android/utils/compiler.h"

#include <stdbool.h>
#include <stddef.h>

ANDROID_BEGIN_HEADER

// CURL library wrapper. This is used to perform the following:
//
// - Initialization and deinitialization of the library, in a multi-threaded
//   context.
// - Download data from a given URL, passing optional POST data to the server.
//
// It is important to avoid exposing <curl/curl.h> to client code, as this
// requires dragging the CURL-specific compiler flags to the client code
// otherwise.

// Initialize CURL library. |ca_info| is an option SSL Certificate Authority
// bundle. Return true on success, false otherwise. NOTE: not thread-safe.
extern bool curl_init(const char* ca_info);

// De-initialize CURL library. Not thread-safe either.
extern void curl_cleanup();

// Call this function instead of curl_easy_init(), because it will add the
// |ca_info| parameter that was passed to curl_init() to the returned CURL
// instance. Note that this function returns a generic pointer that must be
// cast to CURL* by the caller. This is because we don't want to include
// <curl/curl.h> from this header, since it would drag this dependency
// into any client code that includes it, most of them don't need it.
// (note that this requires client code to use $(LIBCURL_CFLAGS) too to
// properly compile on Windows).
extern void* curl_easy_default_init(char** error);

// A CURL-style write callback. Receives the data downloaded from the URL
// inside a curl_download() call. Must return the number of bytes that were
// actually read from |ptr|. |size| is the size of each item in bytes, and
// |nmemb| is the number of items. |userdata| is a client-provided opaque
// pointer.
typedef size_t (*CurlWriteCallback)(char* ptr,
                                    size_t size,
                                    size_t nmemb,
                                    void* userdata);

// Download a file from a given |url|, passing optional POST fields in
// |post_fields|, which can be NULL if not needed. The data is sent to
// the |callback_func| function, which will receive |callback_userdata| as
// its fourth parameter.
//
// This function can block forever.
//
// On failure, return false and sets |*error| to a heap-allocated error message
// string that must be free()-ed by the caller. On success, return true and
// do not touch |*error|.
extern bool curl_download(const char* url,
                          const char* post_fields,
                          CurlWriteCallback callback_func,
                          void* callback_userdata,
                          char** error);

// A variant of curl_download() that throws the downloaded file to /dev/null.
// This is really used to check that a file is available, or to send a POST
// request if |post_fields| is not NULL. If |allow_404| is true, then an
// HTTP response of 404 (file not found) is allowed, as this is used by the
// Google Tools Toolbar API by design. On faillure, return false and sets
// |*error|. On success, return true.
//
// This function can block forever.
extern bool curl_download_null(const char* url,
                               const char* post_fields,
                               bool allow_404,
                               char** error);

ANDROID_END_HEADER
