// Copyright 2014 The Android Open Source Project
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

#include "android/base/TypeTraits.h"

#include <string>
#include <string.h>

namespace android {
namespace base {

// A StringView is a simple (address, size) pair that points to an
// existing read-only string. It's a convenience class used to prevent
// the creation of std::string() objects un-necessarily.
//
// Consider the two following functions:
//
//     size_t count1(const std::string& str) {
//         size_t count = 0;
//         for (size_t n = 0; n < str.size(); ++n) {
//              if (str[n] == '1') {
//                  count++;
//              }
//         }
//         return count;
//     }
//
//     size_t count2(const StringView& str) {
//         size_t count = 0;
//         for (size_t n = 0; n < str.size(); ++n) {
//              if (str[n] == '2') {
//                  count++;
//              }
//         }
//         return count;
//     }
//
// Then consider the following calls:
//
//       size_t n1 = count1("There is 1 one in this string!");
//       size_t n2 = count2("I can count 2 too");
//
// In the first case, the compiler will silently create a temporary
// std::string object, copy the input string into it (allocating memory in
// the heap), call count1() and destroy the std::string upon its return.
//
// In the second case, the compiler will create a temporary StringView,
// initialize it trivially before calling count2(), this results in
// much less generated code, as well as better performance.
//
// Generally speaking, always use a reference or pointer to StringView
// instead of a std::string if your function or method doesn't need to modify
// its input.
//
class StringView {
public:
    constexpr StringView() : mString(""), mSize(0U) {}

    constexpr StringView(const StringView& other) :
        mString(other.data()), mSize(other.size()) {}

    // IMPORTANT: all StringView constructors are intentionally not explict
    // it is needed to allow seamless creation of StringView from all types
    // of strings around - as it's intended to be a generic string wrapper

    // A constexpr constructor from a constant buffer, initializing |mSize|
    // as well. This allows one to declare a static const StringView instance
    // and initialize it at compile time, with no runtime overhead:
    //
    // static constexpr StringView message = "blah";
    //
    template <size_t size>
    constexpr StringView(const char (&buf)[size]) :
        mString(buf), mSize(size - 1) {}

    // Ctor for non-const arrays, AKA buffers. These usually contain some
    // string formatted at runtime, so call strlen() instead of using the
    // buffer size.
    template <size_t size>
    constexpr StringView(char (&buf)[size]) :
        mString(buf), mSize(strlen(buf)) {}

    // Constructor from a const char pointer. It has to be templated to make
    // sure the array-based one is chosen for an array - otherwise non-templated
    // overload always wins
    // Note: the parameter type is a const reference to a const pointer. This
    //   is to make this overload a poorer choice for the case of an array. For
    //   the 'const char[]' argument both 'reference to an array' and 'pointer'
    //   overloads are tied, so compiler can't choose without help
    // Note2: for all constructors and set() calls, |end| must be
    //   dereferencable. It is notrequired to be '\0', but there has to be some
    //   data there. One may not construct a StringView passing past-the-end
    //   iterator as |end|! StringView will try to dereference it.
    template <class Char, class = enable_if<std::is_same<Char, char>>>
    constexpr StringView(const Char* const & string) :
            mString(string ? string : ""), mSize(string ? strlen(string) : 0) {}

    StringView(const std::string& str) :
        mString(str.c_str()), mSize(str.size()) {}

    constexpr StringView(const char* str, size_t len)
        : mString(str ? str : ""), mSize(len) {}

    constexpr StringView(const char* begin, const char* end)
        : mString(begin ? begin : ""), mSize(begin ? end - begin : 0) {}

    constexpr StringView(std::nullptr_t) :
            mString(""), mSize(0) {}

    constexpr const char* c_str() const { return mString; }
    constexpr const char* str() const { return mString; }
    constexpr const char* data() const { return mString; }
    constexpr size_t size() const { return mSize; }

    typedef const char* iterator;
    typedef const char* const_iterator;

    constexpr const_iterator begin() const { return mString; }
    constexpr const_iterator end() const { return mString + mSize; }

    constexpr bool empty() const { return !size(); }
    constexpr bool isNullTerminated() const { return *end() == '\0'; }

    void clear() {
        mSize = 0;
        mString = "";
    }

    constexpr char operator[](size_t index) const {
        return mString[index];
    }

    void set(const char* data, size_t len) {
        mString = data ? data : "";
        mSize = len;
    }

    void set(const char* str) {
        mString = str ? str : "";
        mSize = ::strlen(mString);
    }

    void set(const StringView& other) {
        mString = other.mString;
        mSize = other.mSize;
    }

    // Compare with another StringView.
    int compare(const StringView& other) const;

    StringView& operator=(const StringView& other) {
        set(other);
        return *this;
    }

    // Convert to std::string when needed.
    operator std::string() const { return std::string(mString, mSize); }

private:
    const char* mString;
    size_t mSize;
};

// Comparison operators. Defined as functions to allow automatic type
// conversions with C strings and std::string objects.

bool operator==(const StringView& x, const StringView& y);

inline bool operator!=(const StringView& x, const StringView& y) {
    return !(x == y);
}

inline bool operator<(const StringView& x, const StringView& y) {
    return x.compare(y) < 0;
}

inline bool operator>=(const StringView& x, const StringView& y) {
    return !(x < y);
}

inline bool operator >(const StringView& x, const StringView& y) {
    return x.compare(y) > 0;
}

inline bool operator<=(const StringView& x, const StringView& y) {
    return !(x > y);
}

}  // namespace base
}  // namespace android
