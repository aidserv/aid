// Copyright (C) 2016 The Android Open Source Project
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

#include "android/base/Compiler.h"
#include "android/base/async/Looper.h"
#include "android/base/system/System.h"
#include "android/base/threads/ParallelTask.h"
#include "android/emulation/control/AdbInterface.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace android {
namespace emulation {

class ApkInstaller {
public:
    enum class Result {
        kSuccess,
        kOperationInProgress,
        kApkPermissionsError,
        kAdbConnectionFailed,
        kInstallFailed,
        kUnknownError
    };

    using ResultCallback =
            std::function<void(Result result,
                               android::base::StringView errorString)>;

    explicit ApkInstaller(AdbInterface* adb);
    ~ApkInstaller();

    // Asynchronously installs the apk at |apkFilePath| on the device
    // and invokes |resultCallback| on the calling thread when done.
    // Returns a shared pointer to the AdbCommand that handles the install
    // process.
    AdbCommandPtr install(android::base::StringView apkFilePath,
                          ApkInstaller::ResultCallback resultCallback);

    // Parses the contents of |output| stream as if it was the output from
    // running "adb install" to see if the install succeeded. Returns true
    // if the install succeeded, false otherwise. If the install failed,
    // |outErrorString| is populated with the error from "adb install".
    static bool parseOutputForFailure(std::ifstream& output,
                                      std::string* outErrorString);

    static const char kDefaultErrorString[];

private:
    static const android::base::System::Duration kInstallTimeoutMs;

private:
    AdbCommandPtr mInstallCommand;
    AdbInterface* mAdb;

    DISALLOW_COPY_AND_ASSIGN(ApkInstaller);
};

}  // namespace emulation
}  // namespace android