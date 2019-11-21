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

#include "android/base/testing/TestLooper.h"
#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"
#include "android/base/testing/TestSystem.h"
#include "android/emulation/control/TestAdbInterface.h"
#include "android/utils/path.h"

#include <gtest/gtest.h>

#include <atomic>
#include <string>
#include <utility>
#include <vector>

using android::base::PathUtils;
using android::base::StringView;
using android::base::System;
using android::emulation::TestAdbInterface;
using android::emulation::FilePusher;
using std::shared_ptr;
using std::string;
using std::vector;

class FilePusherTest : public testing::Test {
public:
    using ResultPair = std::pair<string, FilePusher::Result>;
    using ProgressPair = std::pair<double, bool>;

    void SetUp() override {
        mTestSystem.reset(new android::base::TestSystem(
                PATH_SEP "progdir", System::kProgramBitness, PATH_SEP "homedir", PATH_SEP "appdir"));
        mLooper = new android::base::TestLooper();
        mAdb.reset(new TestAdbInterface(mLooper, "adb"));
        mFilePusher.reset(
            new FilePusher(mAdb.get(),
                           [this](StringView filePath, FilePusher::Result result) {
                               mResults.push_back({filePath, result});
                               mAtomicNumCommands--;
                            },
                            [this](double progress, bool done) {
                                mProgresses.push_back({progress, done});
                            }));
        // By default, the 'adb push' command will always block.
        mTestSystem->setShellCommand(&FilePusherTest::fakeShellCommand, this);
        mAtomicNumCommands = 0;
    }

    void TearDown() {
        mFilePusher.reset();
        mAdb.reset();
        delete mLooper;
        mTestSystem.reset();
    }

    static bool fakeShellCommand(void* opaque,
                                 const vector<string>& command,
                                 System::Duration,
                                 System::ProcessExitCode* outExitCode,
                                 System::Pid*,
                                 const string&) {
        EXPECT_GE(command.size(), 4U);
        EXPECT_EQ("adb", command[0]);
        EXPECT_EQ("-s", command[1]);
        EXPECT_EQ("emulator-0", command[2]);
        EXPECT_EQ("push", command[3]);

        auto thisPtr = static_cast<FilePusherTest*>(opaque);
        while(thisPtr->mAtomicNumCommands <= 0) {
            System::get()->sleepMs(2);
        }
        if (outExitCode) {
            *outExitCode = thisPtr->mFakeExitCode;
        }
        return thisPtr->mFakeRunCommandResult;
    }

    void looperAdvance(int numCommands) {
        mAtomicNumCommands = numCommands;
        do {
            auto now = mTestSystem->getHighResTimeUs();
            now += 1000;
            mLooper->runOneIterationWithDeadlineMs(now / 1000);
            mTestSystem->setUnixTimeUs(now);
        } while (mAtomicNumCommands > 0);
    }

    void createAndPushFiles(const vector<std::pair<string, bool>>& files) {
        std::vector<std::pair<std::string, std::string>> push_pairs;
        for (const auto& f : files) {
            if (f.second) {
                EXPECT_TRUE(mTestSystem->getTempRoot()->makeSubFile(f.first.c_str()));
            }
            push_pairs.push_back(std::make_pair(f.first, PATH_SEP "tmp"));
        }
        mFilePusher->pushFiles(push_pairs);
    }

protected:
    std::unique_ptr<android::base::TestSystem> mTestSystem;
    android::base::TestLooper* mLooper;
    std::unique_ptr<TestAdbInterface> mAdb;
    std::unique_ptr<FilePusher> mFilePusher;

    // Fake runCommand support.
    vector<ResultPair> mResults;
    vector<ProgressPair> mProgresses;
    System::ProcessExitCode mFakeExitCode = 0;
    bool mFakeRunCommandResult = true;
    std::atomic_int mAtomicNumCommands;
};

TEST_F(FilePusherTest, success) {
    const vector<ProgressPair> kExpectedProgress = {
            {0, false}, {0.25, false}, {0.5, false},
            {0.75, false}, {1.0, true}};
    const vector<ResultPair> kExpectedResults = {
            {PATH_SEP "file1", FilePusher::Result::Success},
            {PATH_SEP "file2", FilePusher::Result::Success},
            {PATH_SEP "file3", FilePusher::Result::Success},
            {PATH_SEP "file4", FilePusher::Result::Success}};
    createAndPushFiles({
        {PATH_SEP "file1", true},
        {PATH_SEP "file2", true},
        {PATH_SEP "file3", true},
        {PATH_SEP "file4", true}});
    looperAdvance(4);
    EXPECT_EQ(kExpectedProgress, mProgresses);
    EXPECT_EQ(kExpectedResults, mResults);
}

TEST_F(FilePusherTest, fileNotReadable) {
    const vector<ProgressPair> kExpectedProgress = {
            {0, false}, {0.5, false}, {1.0, true}};
    const vector<ResultPair> kExpectedResults = {
            {PATH_SEP "file1", FilePusher::Result::FileReadError},
            {PATH_SEP "file2", FilePusher::Result::Success}};
    createAndPushFiles({
        {PATH_SEP "file1", false},
        {PATH_SEP "file2", true}});

    // Only file2 will actually be pushed.
    looperAdvance(1);
    EXPECT_EQ(kExpectedProgress, mProgresses);
    EXPECT_EQ(kExpectedResults, mResults);
}


TEST_F(FilePusherTest, commandFailures) {
    const vector<ProgressPair> kExpectedProgress = {
            {0, false}, {0.25, false}, {0.5, false}, {0.75, false}, {1.0, true}};
    const vector<ResultPair> kExpectedResults = {
            {PATH_SEP "file1", FilePusher::Result::Success},
            {PATH_SEP "file2", FilePusher::Result::UnknownError},
            {PATH_SEP "file3", FilePusher::Result::AdbPushFailure},
            {PATH_SEP "file4", FilePusher::Result::Success}};
    createAndPushFiles({
        {PATH_SEP "file1", true},
        {PATH_SEP "file2", true},
        {PATH_SEP "file3", true},
        {PATH_SEP "file4", true}});

    looperAdvance(1);
    mFakeRunCommandResult = false;
    looperAdvance(1);
    mFakeRunCommandResult = true;
    mFakeExitCode = 99;
    looperAdvance(1);
    mFakeRunCommandResult = true;
    mFakeExitCode = 0;
    looperAdvance(1);

    EXPECT_EQ(kExpectedProgress, mProgresses);
    EXPECT_EQ(kExpectedResults, mResults);
}


TEST_F(FilePusherTest, enqueueWhilePushing) {
    const vector<ProgressPair> kExpectedProgress =
        { {0, false}, {0.5, false}, {0.5, false}, {0.75, false}, {1, true} };
    const vector<ResultPair> kExpectedResults = {
            {PATH_SEP "file1", FilePusher::Result::Success},
            {PATH_SEP "file2", FilePusher::Result::Success},
            {PATH_SEP "file3", FilePusher::Result::Success},
            {PATH_SEP "file4", FilePusher::Result::Success}};
    createAndPushFiles({
        {PATH_SEP "file1", true},
        {PATH_SEP "file2", true}});
    looperAdvance(1);

    createAndPushFiles({
        {PATH_SEP "file3", true},
        {PATH_SEP "file4", true}});
    looperAdvance(3);

    EXPECT_EQ(kExpectedProgress, mProgresses);
    EXPECT_EQ(kExpectedResults, mResults);
}

TEST_F(FilePusherTest, cancelWhilePushing) {
    const vector<ProgressPair> kExpectedProgress = {{0, false}, {1.0 / 3, false}};
    const vector<ResultPair> kExpectedResults = {
            {PATH_SEP "file1", FilePusher::Result::Success},
    };
    createAndPushFiles({
        {PATH_SEP "file1", true},
        {PATH_SEP "file2", true},
        {PATH_SEP "file3", true}});
    looperAdvance(1);
    mFilePusher->cancel(); // This will cancel file2 and file3.
    EXPECT_EQ(kExpectedProgress, mProgresses);
    EXPECT_EQ(kExpectedResults, mResults);
}
