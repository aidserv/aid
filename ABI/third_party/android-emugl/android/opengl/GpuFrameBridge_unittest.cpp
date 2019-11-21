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

#include "android/opengl/GpuFrameBridge.h"

#include "android/base/async/Looper.h"
#include "android/base/Log.h"
#include "android/base/memory/ScopedPtr.h"

#include <gtest/gtest.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

namespace android {
namespace opengl {

using android::base::ScopedPtr;
using android::base::Looper;

namespace {

struct Frame {
    Frame(int w, int h, const void* pixels) {
        this->width = w;
        this->height = h;
        this->pixels = ::malloc(w * 4 * h);
        ::memcpy(this->pixels, pixels, w * h * 4);
    }

    ~Frame() {
        ::free(this->pixels);
    }

    int width;
    int height;
    void* pixels;
};

class FrameList {
public:
    FrameList() : mCount(0) {}

    ~FrameList() {
        for (int n = mCount; n > 0; --n) {
            delete mFrames[n - 1];
        }
    }

    int count() const { return mCount; }

    Frame* popFront() {
        if (mCount == 0) {
            return NULL;
        }
        Frame* result = mFrames[0];
        mCount--;
        ::memmove(&mFrames[0], &mFrames[1], mCount * sizeof(Frame*));
        mFrames[mCount] = NULL;
        return result;
    }

    const Frame* get(int index) const {
        if (index >= 0 && index < mCount) {
            return mFrames[index];
        } else {
            return NULL;
        }
    }

    static void add(void* context, int w, int h, const void* pixels) {
        FrameList* list = reinterpret_cast<FrameList*>(context);
        CHECK(list->mCount < kMaxFrames);
        Frame* frame = new Frame(w, h, pixels);
        list->mFrames[list->mCount++] = frame;
    }

private:
    enum {
        kMaxFrames = 128
    };

    int mCount;
    Frame* mFrames[kMaxFrames];
};

}  // namespace

TEST(GpuFrameBridge, postFrameWithinSingleThread) {
    ScopedPtr<Looper> looper(Looper::create());
    ASSERT_TRUE(looper.get());

    FrameList list;
    GpuFrameBridge* bridge =
            GpuFrameBridge::create(looper.get(), FrameList::add, &list);
    EXPECT_TRUE(bridge);

    static const unsigned char kFrame0[4] = {
        0xff, 0x80, 0x40, 0xff,
    };

    bridge->postFrame(1, 1, kFrame0);

    EXPECT_EQ(ETIMEDOUT, looper->runWithTimeoutMs(100));

    EXPECT_EQ(1, list.count());
    ScopedPtr<Frame> frame(list.popFront());
    EXPECT_TRUE(frame.get());
    EXPECT_EQ(1, frame->width);
    EXPECT_EQ(1, frame->height);
    for (size_t n = 0; n < sizeof(kFrame0); ++n) {
        EXPECT_EQ(kFrame0[n], reinterpret_cast<unsigned char*>(frame->pixels)[n])
                << "# " << n;
    }
}

}  // namespace opengl
}  // namespace android
