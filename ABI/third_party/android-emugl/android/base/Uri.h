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

#include "android/base/StringView.h"
#include "android/base/StringFormat.h"

#include <string>
#include <type_traits>
#include <utility>

namespace android {
namespace base {

class Uri {
public:
    // |Encode| is aggressive -- it will always encode a reserved character,
    // disregarding a possibly included URL scheme.
    static std::string Encode(StringView uri);

    // |Decode| is aggressive. It will decode every occurance of %XX in a single
    // pass -- even for unreserved characters.
    // Returns empty string on error.
    static std::string Decode(StringView uri);

    // Set of functions for arguments encoding
    struct FormatHelper {
        // Anything which can potentially have encodable character goes here and
        // is encoded into a const char*
        static std::string encodeArg(StringView str);

        // Forward the rest as-is (non-StringView types)
        template <class T>
        static T&& encodeArg(T&& t,
                typename std::enable_if<
                    !std::is_convertible<
                        typename std::decay<T>::type, StringView
                    >::value
                >::type* = nullptr) {

            // Don't allow single char parameters as they have a '%c' format
            // specifier but potentially may encode into a whole string. This
            // is very confusing for the user (which format to use - %c or %s?)
            // and is too error-prone (how to make sure no one later
            // 'fixes' 'wrong' format?)
            static_assert(!std::is_same<typename std::decay<T>::type, char>::value,
                "Uri::FormatEncodeArguments() does not support arguments of type 'char'");

            return std::forward<T>(t);
        }
    };

    // A small convenience method to encode all arguments when formatting the
    // string, but don't touch the |format| string itself
    template <class... Args>
    static std::string FormatEncodeArguments(const char* format,
                                             Args&&... args) {
        return android::base::StringFormat(
                    format,
                    FormatHelper::encodeArg(std::forward<Args>(args))...);
    }
};

}  // namespace base
}  // namespace android
