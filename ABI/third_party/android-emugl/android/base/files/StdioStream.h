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

#include "android/base/Compiler.h"
#include "android/base/files/Stream.h"

#include <stdio.h>

namespace android {
namespace base {

// An implementation of android::base::Stream interface on top of an
// stdio FILE* instance.
class StdioStream : public Stream {
public:
    enum Ownership { kNotOwner, kOwner };

    StdioStream(FILE* file, Ownership ownership = kNotOwner);
    StdioStream(StdioStream&& other);

    virtual ~StdioStream();
    virtual ssize_t read(void* buffer, size_t size) override;
    virtual ssize_t write(const void* buffer, size_t size) override;

    FILE* get() const { return mFile; }

private:
    StdioStream();  // no default constructor
    DISALLOW_COPY_AND_ASSIGN(StdioStream);

    FILE* mFile;
    Ownership mOwnership;
};

}  // namespace base
}  // namespace android
