// Copyright (C) 2015 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "android/base/system/Win32UnicodeString.h"

#include <gtest/gtest.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

namespace android {
namespace base {

TEST(Win32UnicodeString, DefaultConstructor) {
    Win32UnicodeString str;
    EXPECT_EQ(0u, str.size());
    EXPECT_STREQ(L"", str.c_str());
    EXPECT_TRUE(str.data());
}

TEST(Win32UnicodeString, Constructors) {
    static const struct {
        const char* utf8;
        const wchar_t* utf16;
    } kData[] = {
            {"", L""},
            {"Hello World!", L"Hello World!"},
            {"T\xC3\xA9l\xC3\xA9vision", L"T\xE9l\xE9vision"},
            {"foo\xE1\x80\x80 bar", L"foo\x1000 bar"},
    };
    const size_t kDataSize = ARRAY_SIZE(kData);

    for (size_t n = 0; n < kDataSize; ++n) {
        Win32UnicodeString str1(kData[n].utf8);
        EXPECT_EQ(wcslen(kData[n].utf16), str1.size());
        EXPECT_STREQ(kData[n].utf16, str1.c_str());

        Win32UnicodeString str2(kData[n].utf8, strlen(kData[n].utf8));
        EXPECT_EQ(wcslen(kData[n].utf16), str2.size());
        EXPECT_STREQ(kData[n].utf16, str2.c_str());

        std::string baseStr(kData[n].utf8);
        Win32UnicodeString str3(baseStr);
        EXPECT_EQ(wcslen(kData[n].utf16), str3.size());
        EXPECT_STREQ(kData[n].utf16, str3.c_str());

        size_t utf16Len = wcslen(kData[n].utf16);
        Win32UnicodeString str4(kData[n].utf16);
        EXPECT_EQ(utf16Len, str4.size());
        EXPECT_STREQ(kData[n].utf16, str4.c_str());

        Win32UnicodeString str5 = str4;
        EXPECT_EQ(utf16Len, str5.size());
        EXPECT_STREQ(kData[n].utf16, str5.c_str());

        Win32UnicodeString str6("foo");
        str6 = str5;
        EXPECT_EQ(utf16Len, str6.size());
        EXPECT_STREQ(kData[n].utf16, str6.c_str());
    }
}

TEST(Win32UnicodeString, convertToUtf8) {
    static const struct {
        const char* utf8;
        const wchar_t* utf16;
    } kData[] = {
            {"", L""},
            {"Hello World!", L"Hello World!"},
            {"T\xC3\xA9l\xC3\xA9vision", L"T\xE9l\xE9vision"},
            {"foo\xE1\x80\x80 bar", L"foo\x1000 bar"},
    };
    const size_t kDataSize = ARRAY_SIZE(kData);

    for (size_t n = 0; n < kDataSize; ++n) {
        std::string str1 = Win32UnicodeString::convertToUtf8(kData[n].utf16);
        EXPECT_EQ(strlen(kData[n].utf8), str1.size());
        EXPECT_STREQ(kData[n].utf8, str1.c_str());

        std::string str2 = Win32UnicodeString::convertToUtf8(
                kData[n].utf16, wcslen(kData[n].utf16));
        EXPECT_EQ(strlen(kData[n].utf8), str2.size());
        EXPECT_STREQ(kData[n].utf8, str2.c_str());

        char out[256];
        int len = Win32UnicodeString::convertToUtf8(out, sizeof(out),
                                                    kData[n].utf16);
        EXPECT_EQ(strlen(kData[n].utf8) + 1U, (size_t)len);
        EXPECT_STREQ(kData[n].utf8, out);

        len = Win32UnicodeString::convertToUtf8(out, sizeof(out),
                                                kData[n].utf16,
                                                wcslen(kData[n].utf16));
        EXPECT_EQ((int)strlen(kData[n].utf8), len);
        out[len] = 0;
        EXPECT_STREQ(kData[n].utf8, out);

        if (kData[n].utf8[0] != 0) {
            len = Win32UnicodeString::convertToUtf8(out, 1,
                                                    kData[n].utf16);
            EXPECT_EQ(-1, len);
        }

        len = Win32UnicodeString::convertToUtf8(nullptr, 0,
                                                kData[n].utf16);
        EXPECT_EQ(-1, len);

        len = Win32UnicodeString::convertToUtf8(nullptr, 0,
                                                kData[n].utf16,
                                                wcslen(kData[n].utf16));
        EXPECT_EQ(-1, len);

        len = Win32UnicodeString::calcUtf8BufferLength(kData[n].utf16);
        EXPECT_EQ((int)strlen(kData[n].utf8) + 1, len);

        len = Win32UnicodeString::calcUtf8BufferLength(kData[n].utf16,
                                                      wcslen(kData[n].utf16));
        EXPECT_EQ((int)strlen(kData[n].utf8), len);
    }
}

TEST(Win32UnicodeString, convertFromUtf8) {
    static const struct {
        const char* utf8;
        const wchar_t* utf16;
    } kData[] = {
            {"", L""},
            {"Hello World!", L"Hello World!"},
            {"T\xC3\xA9l\xC3\xA9vision", L"T\xE9l\xE9vision"},
            {"foo\xE1\x80\x80 bar", L"foo\x1000 bar"},
    };
    const size_t kDataSize = ARRAY_SIZE(kData);

    for (size_t n = 0; n < kDataSize; ++n) {
        wchar_t out[256];
        int len = Win32UnicodeString::convertFromUtf8(out, ARRAY_SIZE(out),
                                                      kData[n].utf8);
        EXPECT_EQ((int)wcslen(kData[n].utf16) + 1, len);
        EXPECT_STREQ(kData[n].utf16, out);

        len = Win32UnicodeString::convertFromUtf8(out, ARRAY_SIZE(out),
                                                  kData[n].utf8,
                                                  strlen(kData[n].utf8));
        EXPECT_EQ((int)wcslen(kData[n].utf16), len);
        out[len] = 0;
        EXPECT_STREQ(kData[n].utf16, out);

        if (kData[n].utf16[0] != 0) {
            len = Win32UnicodeString::convertFromUtf8(out, 1, kData[n].utf8);
            EXPECT_EQ(-1, len);
        }

        len = Win32UnicodeString::convertFromUtf8(nullptr, 0, kData[n].utf8);
        EXPECT_EQ(-1, len);

        len = Win32UnicodeString::convertFromUtf8(nullptr, 0,
                                                  kData[n].utf8,
                                                  strlen(kData[n].utf8));
        EXPECT_EQ(-1, len);

        len = Win32UnicodeString::calcUtf16BufferLength(kData[n].utf8);
        EXPECT_EQ((int)wcslen(kData[n].utf16) + 1, len);

        len = Win32UnicodeString::calcUtf16BufferLength(kData[n].utf8,
                                                       strlen(kData[n].utf8));
        EXPECT_EQ((int)wcslen(kData[n].utf16), len);
    }
}

TEST(Win32UnicodeString, appending) {
    static const struct {
        const wchar_t* first;
        const wchar_t* second;
        const wchar_t* result;
    } kData[] = {
        {L"foo", L"bar", L"foobar"},
        {L"", L"bar", L"bar"},
        {L"foo", L"", L"foo"},
        {L"foobar", L" with ice cream", L"foobar with ice cream"},
    };

    for (const auto& data : kData) {
        {
            // Test appending Win32UnicodeString
            Win32UnicodeString first(data.first);
            Win32UnicodeString second(data.second);

            first.append(second);
            EXPECT_EQ(wcslen(data.result), first.size());
            EXPECT_STREQ(data.result, first.c_str());
        }
        {
            // Test appending wchar_t*
            Win32UnicodeString str(data.first);
            str.append(data.second);
            EXPECT_EQ(wcslen(data.result), str.size());
            EXPECT_STREQ(data.result, str.c_str());
        }
        {
            // Test appending wchar_t* with length
            Win32UnicodeString str(data.first);
            str.append(data.second, wcslen(data.second));
            EXPECT_EQ(wcslen(data.result), str.size());
            EXPECT_STREQ(data.result, str.c_str());
        }
        if (wcslen(data.second) > 0) {
            // Test appending with fewer characters
            Win32UnicodeString str(data.first);
            str.append(data.second, wcslen(data.second) - 1);
            EXPECT_EQ(wcslen(data.result) - 1, str.size());
            std::wstring choppedResult(data.result, wcslen(data.result) - 1);
            EXPECT_STREQ(choppedResult.c_str(), str.c_str());
        }
    }
}

}  // namespace base
}  // namespace android
