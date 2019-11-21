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

#include <algorithm>

#include <windows.h>

#include <string.h>

namespace android {
namespace base {

Win32UnicodeString::Win32UnicodeString() : mStr(nullptr), mSize(0u) {}

Win32UnicodeString::Win32UnicodeString(const char* str, size_t len)
    : mStr(nullptr), mSize(0u) {
    reset(StringView(str, len));
}

Win32UnicodeString::Win32UnicodeString(StringView str)
    : mStr(nullptr), mSize(0u) {
    reset(str);
}

Win32UnicodeString::Win32UnicodeString(size_t size) : mStr(nullptr), mSize(0u) {
    resize(size);
}

Win32UnicodeString::Win32UnicodeString(const wchar_t* str)
    : mStr(nullptr), mSize(0u) {
    size_t len = str ? wcslen(str) : 0u;
    resize(len);
    ::memcpy(mStr, str ? str : L"", len * sizeof(wchar_t));
}

Win32UnicodeString::Win32UnicodeString(const Win32UnicodeString& other)
    : mStr(nullptr), mSize(0u) {
    resize(other.mSize);
    ::memcpy(mStr, other.c_str(), other.mSize * sizeof(wchar_t));
}

Win32UnicodeString::~Win32UnicodeString() {
    delete[] mStr;
}

Win32UnicodeString& Win32UnicodeString::operator=(
        const Win32UnicodeString& other) {
    resize(other.mSize);
    ::memcpy(mStr, other.c_str(), other.mSize * sizeof(wchar_t));
    return *this;
}

Win32UnicodeString& Win32UnicodeString::operator=(const wchar_t* str) {
    size_t len = str ? wcslen(str) : 0u;
    resize(len);
    ::memcpy(mStr, str ? str : L"", len * sizeof(wchar_t));
    return *this;
}

wchar_t* Win32UnicodeString::data() {
    if (!mStr) {
        // Ensure the function never returns NULL.
        // it is safe to const_cast the pointer here - user isn't allowed to
        // write into it anyway
        return const_cast<wchar_t*>(L"");
    }
    return mStr;
}

std::string Win32UnicodeString::toString() const {
    return convertToUtf8(mStr, mSize);
}

void Win32UnicodeString::reset(const char* str, size_t len) {
    if (mStr) {
        delete[] mStr;
    }
    const int utf16Len = calcUtf16BufferLength(str, len);
    mStr = new wchar_t[utf16Len + 1];
    mSize = static_cast<size_t>(utf16Len);
    convertFromUtf8(mStr, utf16Len, str, len);
    mStr[mSize] = L'\0';
}

void Win32UnicodeString::reset(StringView str) {
    reset(str.c_str(), str.size());
}

void Win32UnicodeString::resize(size_t newSize) {
    if (newSize == 0) {
        delete [] mStr;
        mStr = nullptr;
        mSize = 0;
    } else if (newSize <= mSize) {
        mStr[newSize] = 0;
        mSize = newSize;
    } else {
        wchar_t* oldStr = mStr;
        mStr = new wchar_t[newSize + 1u];
        size_t copySize = std::min(newSize, mSize);
        ::memcpy(mStr, oldStr ? oldStr : L"", copySize * sizeof(wchar_t));
        mStr[copySize] = L'\0';
        mStr[newSize] = L'\0';
        mSize = newSize;
        delete[] oldStr;
    }
}

void Win32UnicodeString::append(const wchar_t* str) {
    append(str, wcslen(str));
}

void Win32UnicodeString::append(const wchar_t* str, size_t len) {
    // NOTE: This method should be rarely used, so don't try to optimize
    // storage with larger capacity values and exponential increments.
    if (!str || !len) {
        return;
    }
    size_t oldSize = size();
    resize(oldSize + len);
    memmove(mStr + oldSize, str, len * sizeof(wchar_t));
}

void Win32UnicodeString::append(const Win32UnicodeString& other) {
    append(other.c_str(), other.size());
}

wchar_t* Win32UnicodeString::release() {
    wchar_t* result = mStr;
    mStr = nullptr;
    mSize = 0u;
    return result;
}

// static
std::string Win32UnicodeString::convertToUtf8(const wchar_t* str, int len) {
    std::string result;
    const int utf8Len = calcUtf8BufferLength(str, len);
    if (utf8Len > 0) {
        result.resize(static_cast<size_t>(utf8Len));
        convertToUtf8(&result[0], utf8Len, str, len);
        if (len == -1) {
            result.resize(utf8Len - 1);  // get rid of the null-terminator
        }
    }
    return result;
}

// returns the return value of a Win32UnicodeString public conversion function
// from a WinAPI conversion function returned code
static int convertRetVal(int winapiResult) {
    return winapiResult ? winapiResult : -1;
}

// static
int Win32UnicodeString::calcUtf8BufferLength(const wchar_t* str, int len) {
    if (len < 0 && len != -1) {
        return -1;
    }
    if (len == 0) {
        return 0;
    }
    const int utf8Len = WideCharToMultiByte(CP_UTF8,  // CodePage
                                            0,        // dwFlags
                                            str,      // lpWideCharStr
                                            len,      // cchWideChar
                                            nullptr,  // lpMultiByteStr
                                            0,        // cbMultiByte
                                            nullptr,  // lpDefaultChar
                                            nullptr); // lpUsedDefaultChar

    return convertRetVal(utf8Len);
}

// static
int Win32UnicodeString::calcUtf16BufferLength(const char* str, int len) {
    if (len < 0 && len != -1) {
        return -1;
    }
    if (len == 0) {
        return 0;
    }
    const int utf16Len = MultiByteToWideChar(CP_UTF8,  // CodePage
                                             0,        // dwFlags
                                             str,      // lpMultiByteStr
                                             len,      // cbMultiByte
                                             nullptr,  // lpWideCharStr
                                             0);       // cchWideChar

    return convertRetVal(utf16Len);
}

// static
int Win32UnicodeString::convertToUtf8(char* outStr, int outLen,
                                      const wchar_t* str, int len) {
    if (!outStr || outLen < 0 || !str || (len < 0 && len != -1)) {
        return -1;
    }
    if (len == 0) {
        return 0;
    }

    const int utf8Len = WideCharToMultiByte(CP_UTF8,  // CodePage
                                            0,        // dwFlags
                                            str,      // lpWideCharStr
                                            len,      // cchWideChar
                                            outStr,   // lpMultiByteStr
                                            outLen,   // cbMultiByte
                                            nullptr,  // lpDefaultChar
                                            nullptr); // lpUsedDefaultChar
    return convertRetVal(utf8Len);
}

// static
int Win32UnicodeString::convertFromUtf8(wchar_t* outStr, int outLen,
                                        const char* str, int len) {
    if (!outStr || outLen < 0 || !str || (len < 0 && len != -1)) {
        return -1;
    }
    if (len == 0) {
        return 0;
    }

    const int utf16Len = MultiByteToWideChar(CP_UTF8,  // CodePage
                                             0,        // dwFlags
                                             str,      // lpMultiByteStr
                                             len,      // cbMultiByte
                                             outStr,   // lpWideCharStr
                                             outLen);  // cchWideChar
    return convertRetVal(utf16Len);
}

}  // namespace base
}  // namespace android
