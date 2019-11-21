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

#pragma once

#include "android/base/files/Stream.h"
#include "android/base/StringView.h"

#include <vector>
#include <errno.h>

namespace android {
namespace base {

// A convenience android::base::Stream implementation that collects
// all written data into a memory buffer, that can be retrieved with
// its view() method, and cleared with its reset() method.
// read() operations on the stream are forbidden.
class TestMemoryOutputStream : public Stream {
public:
    virtual ssize_t read(void* buffer, size_t len) override {
        errno = EINVAL;
        return -1;
    }

    virtual ssize_t write(const void* buffer, size_t len) override {
        mData.insert(mData.end(), static_cast<const char*>(buffer),
                     static_cast<const char*>(buffer) + len);
        return static_cast<ssize_t>(len);
    }

    StringView view() const { return StringView(mData.data(), mData.size()); }

    void reset() { mData.clear(); }

private:
    std::vector<char> mData;
};

}  // namespace base
}  // namespace android
