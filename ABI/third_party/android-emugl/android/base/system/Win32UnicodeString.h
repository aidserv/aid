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

#pragma once

#ifndef _WIN32
// this file is just empty on non-Win32
#else  // _WIN32

#include "android/base/Compiler.h"
#include "android/base/StringView.h"

#include <string>

#include <wchar.h>

namespace android {
namespace base {

// Helper class used to model a Windows Unicode string, which stores
// text and file paths as a zero-terminated array of UTF-16 code points
// (at least since Windows Vista, before that the encoding was slightly
// different).
//
// This is very intentionally *not* a general purpose class. It should only
// be used to simplify conversions between the Win32 Unicode API and the
// rest of android::base which uses UTF-8 for all Unicode text.
class Win32UnicodeString {
public:
    // Default constructor.
    Win32UnicodeString();

    // Initialize a new instance from UTF-8 text at |str| of |len| bytes.
    // This doesn't try to validate the input, i.e. on error, the instance's
    // content is undefined.
    Win32UnicodeString(const char* str, size_t len);

    // Initialize a new instance from an existing string instance |str|.
    explicit Win32UnicodeString(StringView str);

    // Initialize by reserving enough room for a string of |size| UTF-16
    // codepoints.
    explicit Win32UnicodeString(size_t size);

    // Initialize from a zero-terminated wchar_t array.
    explicit Win32UnicodeString(const wchar_t* str);

    // Copy-constructor.
    Win32UnicodeString(const Win32UnicodeString& other);

    // Destructor.
    ~Win32UnicodeString();

    // Assignment operators.
    Win32UnicodeString& operator=(const Win32UnicodeString& other);
    Win32UnicodeString& operator=(const wchar_t* str);

    // Return pointer to first wchar_t in the string.
    const wchar_t* c_str() const { return mStr ? mStr : L""; }

    // Return pointer to writable wchar_t array. This can never be NULL
    // but no more than size() items should be accessed.
    wchar_t* data();

    // Return size of the string, this is the number of UTF-16 code points
    // stored by the string, which may be larger than the number of actual
    // Unicode characters in it.
    size_t size() const { return mSize; }

    // Convert to a string instance holding the corresponding UTF-8 text.
    std::string toString() const;

    // Return n-th character from string.
    wchar_t operator[](size_t index) const { return mStr[index]; }

    // Reset content from UTF-8 text at |str| or |len| bytes.
    void reset(const char* str, size_t len);

    // Reset content from UTF-8 text at |str|.
    void reset(StringView str);

    // Resize array.
    void resize(size_t newSize);

    // Append at the end of a Win32UnicodeString.
    void append(const wchar_t* other);
    void append(const wchar_t* other, size_t len);
    void append(const Win32UnicodeString& other);

    // Release the Unicode string array to the caller.
    wchar_t* release();

    // Directly convert a Unicode string to UTF-8 text and back.
    // |len| - input length. if set to -1, means the input is null-terminated
    static std::string convertToUtf8(const wchar_t* str, int len = -1);

    ////////////////////////////////////////////////////////////////////////////
    // Be careful when crossing this line. The following functions work with
    // raw buffers of static length, and are much harder to use correctly.
    // You've been warned
    ////////////////////////////////////////////////////////////////////////////

    // Calculate the needed buffer size (in characters) to convert the |str|
    // parameter either to or from UTF8
    // |len| - size of the input. -1 means it is 0-terminated
    // Return value is either the size of the buffer needed to convert the whole
    // input of size |len|, or, if |len| is -1, the size needed to convert a
    // zero-terminated string, including the terminator character, or negative -
    // if the size can't be calculated
    static int calcUtf8BufferLength(const wchar_t* str, int len = -1);
    static int calcUtf16BufferLength(const char* str, int len = -1);

    // The following two functions convert |str| into the output buffer, instead
    // of dynamically allocated class object.
    // |str| - source string to convert, must be not null
    // |len| - length of the source string, in chars; must be positive or -1.
    //  -1 means 'the input is zero-terminated'. In that case output has to
    //  be large enough to hold a zero character as well.
    // |outStr| - the output buffer, must be not null
    // |outLen| - the output buffer length, in chars; must be large enough to
    //  hold the converted string. One can calculate the required length using
    //  one of the getUtf<*>BufferLength() functions
    //
    // Returns a number of bytes written into the |outBuf|, or -1 on failure
    //
    // Note: you can't get the reason of the failure from this function call:
    //  it could be bad input parameter (e.g. null buffer), too short output,
    //  bad characters in the input string, even OS failure. So make sure
    //  you do the call to getUtf<*>BufferLength() and all parameters are
    //  correct - if you need to know the exact reason and not just "can't" one.
    static int convertToUtf8(char* outStr, int outLen,
                              const wchar_t* str, int len = -1);
    static int convertFromUtf8(wchar_t* outStr, int outLen,
                                const char* str, int len = -1);

private:
    wchar_t* mStr;
    size_t mSize;
};

}  // namespace base
}  // namespace android

#endif  // _WIN32
