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

#include "android/base/memory/MallocUsableSize.h"

#include <stdlib.h>

#include <gtest/gtest.h>

namespace android {
namespace base {

#if USE_MALLOC_USABLE_SIZE

static size_t computeSize(size_t index) {
    return 3 + index * 17;
}

TEST(MallocUsableSize, UseMallocUsableSize) {
    const size_t kCount = 1000;
    void* blocks[kCount];
    for (size_t n = 0; n < kCount; ++n) {
        blocks[n] = ::malloc(computeSize(n));
        EXPECT_TRUE(blocks[n]);
    }

    for (size_t n = 0; n < kCount; ++n) {
        size_t size = computeSize(n);
        EXPECT_LE(size, ::malloc_usable_size(blocks[n]))
                << "For block " << n << " of " << size << " bytes";
    }

    for (size_t n = kCount; n > 0; --n) {
        ::free(blocks[n - 1U]);
    }
}

#else  // !USE_MALLOC_USABLE_SIZE

TEST(MallocUsableSize, DoNotUseMallocUsableSize) {
#ifdef __linux__
    EXPECT_TRUE(false) << "Linux should have malloc_usable_size";
#elif defined(__APPLE__)
    EXPECT_TRUE(false) << "Darwin should have malloc_size";
#elif defined(__FreeBSD__)
    EXPECT_TRUE(false) << "FreeBSD should have malloc_size";
#endif
}

#endif  // !USE_MALLOC_USABLE_SIZE

}  // namespace base
}  // namespace android
