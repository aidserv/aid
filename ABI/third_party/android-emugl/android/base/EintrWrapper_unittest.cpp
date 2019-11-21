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

// Forces debug mode
#define EINTR_WRAPPER_DEBUG 1

#include "android/base/EintrWrapper.h"

#include <stdarg.h>
#include <setjmp.h>

#include <gtest/gtest.h>

namespace android {
namespace base {

// Implementation of a custom panic function used to detect that
// HANDLE_EINTR() called panic after too many loop iterations.
// Uses setjmp()/longjmp() since the panic handler must be
// __attribute__((noreturn)).
using namespace ::android::base::testing;

class EintrWrapperTest : public ::testing::Test, LogOutput {
public:
    EintrWrapperTest() :
            mFatal(false),
            mLogged(true),
            mPrevious(LogOutput::setNewOutput(this)) {}

    ~EintrWrapperTest() {
        LogOutput::setNewOutput(mPrevious);
    }

    virtual void logMessage(const android::base::LogParams& params,
                            const char* message,
                            size_t messageLen) {
        mFatal = (params.severity == LOG_FATAL);
        mLogged = true;
        if (mFatal)
            longjmp(mJumper, 1);
    }

protected:
    bool mFatal;
    bool mLogged;
    LogOutput* mPrevious;
    jmp_buf mJumper;
};


// Loop counter used by several functions below.
static int gLoopCount = 0;

// This function returns the first time it is called, or -1/EINVAL
// otherwise.
static int returnEinvalAfterFirstCall(void) {
    if (++gLoopCount == 1)
        return 0;

    errno = EINVAL;
    return -1;
}

TEST_F(EintrWrapperTest, NoLoopOnSuccess) {
    gLoopCount = 0;
    EXPECT_EQ(0, HANDLE_EINTR(returnEinvalAfterFirstCall()));
    EXPECT_EQ(1, gLoopCount);
}

TEST_F(EintrWrapperTest, NoLoopOnRegularError) {
    gLoopCount = 0;
    EXPECT_EQ(0, HANDLE_EINTR(returnEinvalAfterFirstCall()));
    EXPECT_EQ(-1, HANDLE_EINTR(returnEinvalAfterFirstCall()));
    EXPECT_EQ(EINVAL, errno);
    EXPECT_EQ(2, gLoopCount);
}

static int alwaysReturnEintr(void) {
    gLoopCount++;
#ifdef _WIN32
    // Win32 cannot generate EINTR.
    return 0;
#else
    errno = EINTR;
    return -1;
#endif
}

TEST_F(EintrWrapperTest, IgnoreEintr) {
    gLoopCount = 0;
    EXPECT_EQ(0, IGNORE_EINTR(alwaysReturnEintr()));
    EXPECT_EQ(1, gLoopCount);
}

#ifndef _WIN32

// This function loops 10 times around |gLoopCount|, while returning
// -1/errno.
static int loopEintr10(void) {
    if (++gLoopCount < 10) {
        errno = EINTR;
        return -1;
    }
    return 0;
}

TEST_F(EintrWrapperTest, LoopOnEintr) {
    gLoopCount = 0;
    EXPECT_EQ(0, HANDLE_EINTR(loopEintr10()));
    EXPECT_EQ(10, gLoopCount);
}

static int loopEintr200(void) {
    if (++gLoopCount < 200) {
        errno = EINTR;
        return -1;
    }
    return 0;
}

TEST_F(EintrWrapperTest, PanicOnTooManyLoops) {
    gLoopCount = 0;
    if (setjmp(mJumper) == 0) {
        HANDLE_EINTR(loopEintr200());
        ASSERT_TRUE(0) << "HANDLE_EINTR() didn't panic!";
    } else {
        EXPECT_TRUE(mLogged);
        EXPECT_TRUE(mFatal);
        EXPECT_EQ(MAX_EINTR_LOOP_COUNT, gLoopCount);
    }
}

#endif  // !_WIN32

}  // namespace base
}  // namespace android
