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

#include "android/emulation/control/FilePusher.h"

#include "android/base/Compiler.h"
#include "android/base/Log.h"
#include "android/base/system/System.h"

namespace android {
namespace emulation {

using android::base::Looper;
using android::base::ParallelTask;
using android::base::StringView;
using android::base::System;
using std::string;
using std::vector;

FilePusher::FilePusher(AdbInterface* adb,
                       FilePusher::ResultCallback resultCallback,
                       FilePusher::ProgressCallback progressCallback)
    : mAdb(adb),
      mProgressCallback(progressCallback
                                ? std::move(progressCallback)
                                : ProgressCallback([](double, bool) {})),
      mResultCallback(resultCallback
                              ? std::move(resultCallback)
                              : ResultCallback([](StringView, Result) {})) {}

FilePusher::~FilePusher() {
    cancel();
}

void FilePusher::pushFiles(const std::vector<FilePusher::PushItem>& files) {
    mPushQueue.insert(mPushQueue.end(), files.begin(), files.end());
    mNumQueued += files.size();
    if (!mCurrentPushCommand) {
        mProgressCallback(0, false);
        pushNextItem();
    }
}

void FilePusher::cancel() {
    if (mCurrentPushCommand) {
        mCurrentPushCommand->cancel();
        mCurrentPushCommand.reset();
    }
    mPushQueue.clear();
    resetProgress();
}

void FilePusher::pushNextItem() {
    if (mPushQueue.empty()) {
        return;
    }
    LOG(VERBOSE) << "Pushing next item... [Queue length: " << mPushQueue.size()
                 << "]";
    mCurrentItem = mPushQueue.front();
    mPushQueue.pop_front();

    if (!System::get()->pathCanRead(mCurrentItem.first)) {
        pushDone(Result::FileReadError);
        return;
    }
    mCurrentPushCommand = mAdb->runAdbCommand(
        {"push", mCurrentItem.first, mCurrentItem.second},
        [this](const OptionalAdbCommandResult& result) {
            if (!result) {
                pushDone(Result::UnknownError);
            } else if (result->exit_code) {
                pushDone(Result::AdbPushFailure);
            } else {
                pushDone(Result::Success);
            }
        },
        System::kInfinite);
}

void FilePusher::pushDone(const Result& result) {
    bool allDone = mPushQueue.empty();
    ++mNumPushed;
    mResultCallback(mCurrentItem.first, result);
    mProgressCallback(static_cast<double>(mNumPushed) / mNumQueued, allDone);
    if (!allDone) {
        pushNextItem();
    } else {
        mCurrentPushCommand.reset();
        resetProgress();
    }
}

void FilePusher::resetProgress() {
    mNumPushed = 0;
    mNumQueued = 0;
}

}  // namespace emulation
}  // namespace android
