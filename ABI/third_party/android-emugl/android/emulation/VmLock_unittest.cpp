// Copyright 2016 The Android Open Source Project
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

#include "android/emulation/VmLock.h"
#include "android/emulation/testing/TestVmLock.h"

#include <gtest/gtest.h>

namespace android {

TEST(VmLock, Default) {
    VmLock* vmLock = VmLock::get();
    ASSERT_TRUE(vmLock);
    vmLock->lock();
    vmLock->unlock();
}

TEST(VmLock, CustomVmLock) {
    TestVmLock myLock;

    EXPECT_EQ(0, myLock.mLockCount);
    EXPECT_EQ(0, myLock.mUnlockCount);
    EXPECT_FALSE(myLock.isLockedBySelf());

    VmLock::get()->lock();
    EXPECT_EQ(1, myLock.mLockCount);
    EXPECT_EQ(0, myLock.mUnlockCount);
    EXPECT_TRUE(myLock.isLockedBySelf());

    VmLock::get()->unlock();
    EXPECT_EQ(1, myLock.mLockCount);
    EXPECT_EQ(1, myLock.mUnlockCount);
    EXPECT_FALSE(myLock.isLockedBySelf());

    VmLock::get()->lock();
    EXPECT_EQ(2, myLock.mLockCount);
    EXPECT_EQ(1, myLock.mUnlockCount);
    EXPECT_TRUE(myLock.isLockedBySelf());

    VmLock::get()->unlock();
    EXPECT_EQ(2, myLock.mLockCount);
    EXPECT_EQ(2, myLock.mUnlockCount);
    EXPECT_FALSE(myLock.isLockedBySelf());

    myLock.release();

    VmLock::get()->lock();
    EXPECT_EQ(2, myLock.mLockCount);
    EXPECT_EQ(2, myLock.mUnlockCount);
    EXPECT_FALSE(myLock.isLockedBySelf());

    VmLock::get()->unlock();
    EXPECT_EQ(2, myLock.mLockCount);
    EXPECT_EQ(2, myLock.mUnlockCount);
    EXPECT_FALSE(myLock.isLockedBySelf());
}

TEST(ScopedVmLock, Default) {
    ScopedVmLock vlock;
}

TEST(ScopedVmLock, CustomVmLock) {
    TestVmLock myLock;

    EXPECT_EQ(0, myLock.mLockCount);
    EXPECT_EQ(0, myLock.mUnlockCount);

    {
        ScopedVmLock vlock;
        EXPECT_EQ(1, myLock.mLockCount);
        EXPECT_EQ(0, myLock.mUnlockCount);
    }
    EXPECT_EQ(1, myLock.mLockCount);
    EXPECT_EQ(1, myLock.mUnlockCount);

    {
        ScopedVmLock vlock;
        EXPECT_EQ(2, myLock.mLockCount);
        EXPECT_EQ(1, myLock.mUnlockCount);
    }
    EXPECT_EQ(2, myLock.mLockCount);
    EXPECT_EQ(2, myLock.mUnlockCount);

    myLock.release();
    {
        ScopedVmLock vlock;
        EXPECT_EQ(2, myLock.mLockCount);
        EXPECT_EQ(2, myLock.mUnlockCount);
    }
    EXPECT_EQ(2, myLock.mLockCount);
    EXPECT_EQ(2, myLock.mUnlockCount);
}

}  // namespace android
