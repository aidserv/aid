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

#include "android/base/testing/TestLooper.h"
#include "android/base/Compiler.h"
#include "android/base/system/System.h"
#include "android/base/testing/TestSystem.h"
#include "android/emulation/control/TestAdbInterface.h"
#include "android/utils/path.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <memory>
#include <string>
#include <vector>

using android::base::StringView;
using android::base::System;
using android::emulation::ScreenCapturer;
using android::emulation::TestAdbInterface;
using std::string;
using std::vector;

class ScreenCapturerTest : public ::testing::Test {
public:
    // Convenience typedef.
    using Result = ScreenCapturer::Result;

    ScreenCapturerTest()
        : mTestSystem(PATH_SEP "progdir",
                      System::kProgramBitness,
                      PATH_SEP "homedir",
                      PATH_SEP "appdir") {}

    void SetUp() override {
        mInvalidShellCommand = false;
        mHaveResult = false;
        mTestSystem.setShellCommand(&ScreenCapturerTest::shellCmdHandler,
                                    this);
        mLooper = new android::base::TestLooper();
        mAdb.reset(new TestAdbInterface(mLooper, "adb"));
        mCapturer.reset(new ScreenCapturer(mAdb.get()));
        mTestSystem.getTempRoot()->makeSubDir(PATH_SEP "ScreencapOut");
    }

    void TearDown() override {
        // Ensure that no hanging thread makes the object undead at the end of
        // the test.
        mCapturer.reset();
        mAdb.reset();
        delete mLooper;
    }

    void looperAdvance() {
        auto now = mTestSystem.getHighResTimeUs();
        now += 1000;
        mLooper->runOneIterationWithDeadlineMs(now / 1000);
        mTestSystem.setUnixTimeUs(now);
    }

    static bool shellCmdHandler(void* opaque,
                                const vector<string>& commandLine,
                                System::Duration timeoutMs,
                                System::ProcessExitCode* outExitCode,
                                System::Pid* outChildPid,
                                const string& outputFile) {
        return static_cast<ScreenCapturerTest*>(opaque)
                ->shellCmdHandlerImpl(commandLine, timeoutMs,
                                      outExitCode, outChildPid,
                                      outputFile);
    }

    bool shellCmdHandlerImpl(const vector<string>& commandLine,
                             System::Duration,
                             System::ProcessExitCode* outExitCode,
                             System::Pid*,
                             const string&) {
        bool isScreencap =
            std::find(commandLine.begin(), commandLine.end(), "screencap") != commandLine.end();
        bool isPull =
            std::find(commandLine.begin(), commandLine.end(), "pull") != commandLine.end();
        // Always play nice with the exit code.
        if (outExitCode) {
            *outExitCode = 0;
        }
        if (isScreencap && !isPull) {
            return mCaptureMustSucceed;
        } else if (isPull && !isScreencap) {
            return mPullMustSucceed;
        } else {
            mInvalidShellCommand = true;
            return false;
        }
    }

    void resultSaver(Result result) {
        mResult = result;
        mHaveResult = true;
    }

protected:
    android::base::TestSystem mTestSystem;
    android::base::TestLooper* mLooper;
    std::unique_ptr<TestAdbInterface> mAdb;
    std::unique_ptr<ScreenCapturer> mCapturer;
    bool mCaptureMustSucceed;
    bool mPullMustSucceed;
    bool mInvalidShellCommand;
    // Result from the ScreenCapturer callback is saved here.
    Result mResult = Result::kUnknownError;
    std::atomic_bool mHaveResult;

    DISALLOW_COPY_AND_ASSIGN(ScreenCapturerTest);
};

TEST_F(ScreenCapturerTest, screenshotCaptureFailure) {
    mCaptureMustSucceed = false;
    mPullMustSucceed = false;
    mCapturer->capture(PATH_SEP "ScreencapOut",
                       [this](ScreenCapturer::Result result,
                              StringView filePath) { resultSaver(result); });
    while (!mHaveResult) {
        looperAdvance();
    }
    EXPECT_FALSE(mInvalidShellCommand);
    EXPECT_EQ(Result::kCaptureFailed, mResult);
}

TEST_F(ScreenCapturerTest, adbPullFailure) {
    mCaptureMustSucceed = true;
    mPullMustSucceed = false;
    mCapturer->capture(PATH_SEP "ScreencapOut",
                       [this](ScreenCapturer::Result result,
                              StringView filePath) { resultSaver(result); });
    while (!mHaveResult) {
        looperAdvance();
    }
    EXPECT_FALSE(mInvalidShellCommand);
    EXPECT_EQ(Result::kPullFailed, mResult);
}

TEST_F(ScreenCapturerTest, success) {
    mCaptureMustSucceed = true;
    mPullMustSucceed = true;
    mCapturer->capture(PATH_SEP "ScreencapOut",
                       [this](ScreenCapturer::Result result,
                              StringView filePath) { resultSaver(result); });
    while (!mHaveResult) {
        looperAdvance();
    }
    EXPECT_FALSE(mInvalidShellCommand);
    EXPECT_EQ(Result::kSuccess, mResult);
}

TEST_F(ScreenCapturerTest, synchronousFailures) {
    mCapturer->capture(PATH_SEP "ScreencapOut",
                       [this](ScreenCapturer::Result result,
                              StringView filePath) { resultSaver(result); });
    mCapturer->capture(PATH_SEP "ScreencapOut",
                       [this](ScreenCapturer::Result result,
                              StringView filePath) { resultSaver(result); });
    EXPECT_EQ(Result::kOperationInProgress, mResult);
}
