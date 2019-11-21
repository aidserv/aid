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

#include "android/crashreport/CrashSystem.h"

namespace android {
namespace crashreport {

class TestCrashSystem : public CrashSystem {
public:
    TestCrashSystem(const std::string& crashDir, const std::string& crashURL)
        : mPrevCrashSystem(CrashSystem::setForTesting(this)),
          mCrashDir(crashDir),
          mCrashURL(crashURL) {}

    virtual ~TestCrashSystem() { CrashSystem::setForTesting(mPrevCrashSystem); }

    virtual const std::string& getCrashDirectory(void) override {
        return mCrashDir;
    }

    virtual const std::string& getCrashURL(void) override { return mCrashURL; }

private:
    CrashSystem* mPrevCrashSystem;
    std::string mCrashDir;
    std::string mCrashURL;
};

}  // namespace crashreport
}  // namespace android
