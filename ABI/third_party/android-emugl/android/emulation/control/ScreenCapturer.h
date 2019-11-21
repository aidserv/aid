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

#include "android/base/async/Looper.h"
#include "android/base/Compiler.h"
#include "android/base/system/System.h"
#include "android/base/threads/ParallelTask.h"
#include "android/emulation/control/AdbInterface.h"

#include <functional>
#include <string>
#include <memory>
#include <vector>

namespace android {
namespace emulation {

class ScreenCapturer {
public:
    enum class Result {
        kSuccess,
        kOperationInProgress,
        kCaptureFailed,
        kSaveLocationInvalid,
        kPullFailed,
        kUnknownError
    };

    using ResultCallback =
            std::function<void(Result result,
                               android::base::StringView filePath)>;

    explicit ScreenCapturer(AdbInterface* adb) : mAdb(adb) {}
    ~ScreenCapturer();

    // Runs adb commands to capture the screen and pull the screenshot from
    // the device.
    // |outputDirectoryPath| should specify the path to the directory
    // to which the screenshot will be written. |resultCallback| will
    // be invoked upon completion.
    void capture(android::base::StringView outputDirectoryPath,
                 ResultCallback resultCallback);

private:
    void pullScreencap(ResultCallback resultCallback,
                       android::base::StringView outputDirectoryPath);
    static const android::base::System::Duration kPullTimeoutMs;
    static const char kRemoteScreenshotFilePath[];
    static const android::base::System::Duration kScreenCaptureTimeoutMs;

    AdbInterface* mAdb;
    AdbCommandPtr mCaptureCommand;
    AdbCommandPtr mPullCommand;
    const ResultCallback mResultCallback;

    unsigned mNonDefaultCheckTimeoutForTestMs = 0;

    DISALLOW_COPY_AND_ASSIGN(ScreenCapturer);
};

}  // namespace emulation
}  // namespace android
