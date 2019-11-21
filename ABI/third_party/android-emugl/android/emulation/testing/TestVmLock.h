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

namespace android {

class TestVmLock : public VmLock {
public:
    TestVmLock() : mOldVmLock(VmLock::set(this)), mInstalled(true) {}

    ~TestVmLock() { release(); }

    void release() {
        if (mInstalled) {
            // NOTE: A value of nullptr for mOldVmLock is valid.
            VmLock::set(mOldVmLock);
            mInstalled = false;
        }
    }

    virtual void lock() override { mLockCount++; }
    virtual void unlock() override { mUnlockCount++; }
    virtual bool isLockedBySelf() const override {
        return mLockCount > mUnlockCount;
    }

    int mLockCount = 0;
    int mUnlockCount = 0;
    VmLock* mOldVmLock = nullptr;
    bool mInstalled = false;
};

} // namespace android
