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

#include "android/base/StringView.h"
#include "android/base/system/System.h"
#include "android/base/threads/ParallelTask.h"
#include "android/emulation/control/AdbInterface.h"

#include <functional>
#include <deque>
#include <memory>
#include <string>
#include <vector>

namespace android {
namespace emulation {

class FilePusher {
public:
    enum class Result {
        Success,

        UnknownError,
        ProcessStartFailure,
        OperationInProgress,
        // Count not read file to push.
        FileReadError,
        AdbPushFailure,
    };

    using PushItem = std::pair<std::string, std::string>;
    // This callback reports the result of each individual push operation.
    using ResultCallback =
            std::function<void(android::base::StringView filePath,
                               FilePusher::Result result)>;
    // First argument has an double value in [0,1], reporting progress.
    // Second argument reports whether current batch of file pushing has been
    // completed.
    // |enqueue|ing more files can cause the progress to decrease.
    using ProgressCallback = std::function<void(double, bool)>;

    FilePusher(AdbInterface* adb,
               ResultCallback resultCallback,
               ProgressCallback progressCallback);
    ~FilePusher();

    void pushFiles(const std::vector<PushItem>& files);

    // Cancel all outstanding pushes. Note that we do not cancel currently
    // active push.
    void cancel();

private:
    void pushNextItem();
    void pushDone(const Result& result);
    void resetProgress();

    AdbInterface* mAdb;
    AdbCommandPtr mCurrentPushCommand;
    ProgressCallback mProgressCallback;
    ResultCallback mResultCallback;

    std::deque<PushItem> mPushQueue;
    PushItem mCurrentItem;
    size_t mNumQueued = 0;
    size_t mNumPushed = 0;

    DISALLOW_COPY_AND_ASSIGN(FilePusher);
};

}  // namespace emulation
}  // namespace android
