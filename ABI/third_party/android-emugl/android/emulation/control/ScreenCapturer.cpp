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

#include "android/emulation/control/ScreenCapturer.h"

#include "android/base/files/PathUtils.h"
#include "android/base/StringFormat.h"
#include "android/base/system/System.h"
#include "android/base/threads/Async.h"

namespace android {
namespace emulation {

using android::base::Looper;
using android::base::ParallelTask;
using android::base::StringView;
using android::base::System;
using std::string;
using std::vector;

// static
const System::Duration ScreenCapturer::kPullTimeoutMs = 5000;
const char ScreenCapturer::kRemoteScreenshotFilePath[] =
        "/data/local/tmp/screen.png";
// If you run screencap while using emulator over remote desktop, this can
// take... a while.
const System::Duration ScreenCapturer::kScreenCaptureTimeoutMs =
        System::kInfinite;

ScreenCapturer::~ScreenCapturer() {
    if (mCaptureCommand) {
        mCaptureCommand->cancel();
    }
    if (mPullCommand) {
        mPullCommand->cancel();
    }
}

void ScreenCapturer::capture(StringView outputDirectoryPath,
                             ResultCallback resultCallback) {
    if (mCaptureCommand || mPullCommand) {
        resultCallback(Result::kOperationInProgress, nullptr);
        return;
    }
    std::string out_path = outputDirectoryPath;
    mCaptureCommand =
        mAdb->runAdbCommand(
            {"shell", "screencap", "-p", kRemoteScreenshotFilePath },
            [this, resultCallback, out_path](const OptionalAdbCommandResult& result) {
                if (!result || result->exit_code) {
                    resultCallback(Result::kCaptureFailed, nullptr);
                } else {
                    pullScreencap(resultCallback, out_path);
                }
                mCaptureCommand.reset();
            },
            kScreenCaptureTimeoutMs,
            false);
}

void ScreenCapturer::pullScreencap(ResultCallback resultCallback,
                                   StringView outputDirectoryPath) {
    if (!System::get()->pathIsDir(outputDirectoryPath) ||
        !System::get()->pathCanWrite(outputDirectoryPath)) {
        resultCallback(Result::kSaveLocationInvalid, nullptr);
    } else {
        auto fileName =
            android::base::StringFormat(
                "Screenshot_%lu.png",
                static_cast<unsigned long>(System::get()->getUnixTime()));
        auto filePath =
                android::base::PathUtils::join(outputDirectoryPath, fileName);
        mPullCommand = mAdb->runAdbCommand(
                {"pull", kRemoteScreenshotFilePath, filePath},
                [this, resultCallback,
                 filePath](const OptionalAdbCommandResult& result) {
                    resultCallback((!result || result->exit_code)
                                           ? Result::kPullFailed
                                           : Result::kSuccess,
                                   filePath);
                    mPullCommand.reset();
                },
                kPullTimeoutMs, false);
    }
}

}  // namespace emulation
}  // namespace android
