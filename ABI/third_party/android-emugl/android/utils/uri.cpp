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

// Some free functions for manipulating strings as URIs. Wherever possible,
// these functions take const references to StringView to avoid unnecessary
// copies.

#include "android/utils/uri.h"

#include "android/base/misc/StringUtils.h"
#include "android/base/Uri.h"

using android::base::Uri;
using android::base::strDup;

char* uri_encode(const char* uri) {
    return strDup(Uri::Encode(uri));
}

char* uri_decode(const char* uri) {
    return strDup(Uri::Decode(uri));
}
