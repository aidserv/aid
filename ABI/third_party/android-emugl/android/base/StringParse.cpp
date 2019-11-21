// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/StringParse.h"

#include "android/base/StringView.h"

#ifdef _WIN32
#include <algorithm>
#include <string>
#else
#include "android/base/memory/LazyInstance.h"
#include "android/base/memory/ScopedPtr.h"
#endif

#include <assert.h>
#include <locale.h>
#include <string.h>
#include <stdlib.h>

#ifdef __APPLE__
#include <xlocale.h>
#endif

namespace android {
namespace base {

#ifdef _WIN32

StringView fixStringForFloats(StringView string,
                              StringView format,
                              std::string* fixedString) {
    static constexpr char floatingPointFormats[][3] = {"%f", "%g", "%a", "%E",
                                                       "%e"};
    static constexpr char decimalDot = '.';
    const StringView decimalPoint = localeconv()->decimal_point;
    if (decimalPoint == ".") {
        return string;
    }

    const bool hasDecimalDot =
            std::find(string.begin(), string.end(), decimalDot);
    if (!hasDecimalDot) {
        return string;
    }

    const bool hasFloatingPointFormat = std::any_of(
            std::begin(floatingPointFormats), std::end(floatingPointFormats),
            [format](const char(&fp)[3]) {
                return format.end() != std::search(format.begin(), format.end(),
                                                   std::begin(fp),
                                                   std::prev(std::end(fp)));
            });
    if (!hasFloatingPointFormat) {
        return string;
    }

    // Need to replace the '.' with whatever we've got in the current locale.
    for (const char c : string) {
        if (c == decimalDot) {
            fixedString->append(decimalPoint.begin(), decimalPoint.end());
        } else {
            fixedString->push_back(c);
        }
    }
    return *fixedString;
}

#else  // !_WIN32

struct GlobalCLocaleT {
    locale_t value = newlocale(LC_ALL_MASK, "C", (locale_t)0);
};
static LazyInstance<GlobalCLocaleT> sGlobalCLocale;

#endif  // !_WIN32

extern "C" int SscanfWithCLocale(const char* string, const char* format, ...) {
    va_list args;
    va_start(args, format);
    const int res = ::SscanfWithCLocaleWithArgs(string, format, args);
    va_end(args);
    return res;
}

extern "C" int SscanfWithCLocaleWithArgs(const char* string,
                                         const char* format,
                                         va_list args) {
#ifdef _WIN32

    // MinGW doesn't implement per-thread locales, and it's just too dangerous
    // to change global locale for a short period of time.
    // That's why let's do it differently: if the floating point separator isn't
    // '.', and there's a %f/%g/%a in the format string, replace the incoming
    // dot with the expected decimal separator in |string|.

    std::string fixedString;
    string = fixStringForFloats(string, format, &fixedString).c_str();

#else  // !_WIN32

    const auto locale = sGlobalCLocale->value;
    const auto scopedCLocale = makeCustomScopedPtr(
            locale ? uselocale(locale) : nullptr, uselocale);

#endif  // !_WIN32
    return vsscanf(string, format, args);
}

}  // namespace base
}  // namespace android
