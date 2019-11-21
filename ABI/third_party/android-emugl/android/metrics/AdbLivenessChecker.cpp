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

#include "android/metrics/AdbLivenessChecker.h"

#include "android/base/system/System.h"
#include "android/base/files/PathUtils.h"
#include "android/base/Log.h"
#include "android/base/threads/ParallelTask.h"
#include "android/emulation/ConfigDirs.h"

#include "android/metrics/proto/studio_stats.pb.h"

namespace android {
namespace metrics {

using android::base::PathUtils;
using android::base::System;
using android::ConfigDirs;
using std::shared_ptr;

static const char kAdbLivenessKey[] = "adb_liveness";
static const int kMaxAttempts = 3;

// static
AdbLivenessChecker::Ptr AdbLivenessChecker::create(
        android::emulation::AdbInterface* adb,
        android::base::Looper* looper,
        MetricsReporter* reporter,
        android::base::StringView emulatorName,
        android::base::Looper::Duration checkIntervalMs) {
    auto inst = Ptr(new AdbLivenessChecker(adb, looper, reporter, emulatorName,
                                           checkIntervalMs));
    return inst;
}

AdbLivenessChecker::AdbLivenessChecker(
        android::emulation::AdbInterface* adb,
        android::base::Looper* looper,
        MetricsReporter* reporter,
        android::base::StringView emulatorName,
        android::base::Looper::Duration checkIntervalMs)
    : mAdb(adb),
      mLooper(looper),
      mReporter(reporter),
      mEmulatorName(emulatorName),
      mCheckIntervalMs(checkIntervalMs),
      // We use raw pointer to |this| instead of a shared_ptr to avoid cicrular
      // ownership. mRecurrentTask promises to cancel any outstanding tasks when
      // it's destructed.
      mRecurrentTask(looper,
                     [this]() { return adbCheckRequest(); },
                     mCheckIntervalMs),
      mRemainingAttempts(kMaxAttempts) {
    // Don't call start() here: start() launches a parallel task that calls
    // shared_from_this(), which needs at least one shared pointer owning
    // |this|. We can't guarantee that until the constructor call returns.
}

AdbLivenessChecker::~AdbLivenessChecker() {
    if (mDevicesCommand) {
        mDevicesCommand->cancel();
    }
    if (mShellExitCommand) {
        mShellExitCommand->cancel();
    }
    stop();
}

void AdbLivenessChecker::start() {
    mRecurrentTask.start();
}

void AdbLivenessChecker::stop() {
    mRecurrentTask.stopAndWait();
}

bool AdbLivenessChecker::adbCheckRequest() {
    if (!mRemainingAttempts) {
        LOG(VERBOSE) << "Quitting adb liveness check";
        return false;
    }

    if (mIsCheckRunning) {
        return true;
    }
    mIsCheckRunning = true;

#if 0
    // NOTE: Capture a shared_ptr to |this| to guarantee object lifetime while
    // the parallel task is running.
    auto weakSelf = std::weak_ptr<AdbLivenessChecker>(shared_from_this());
    auto taskFunction = [weakSelf](CheckResult* outResult) {
        if (const auto self = weakSelf.lock()) {
            self->runCheckBlocking(outResult);
        }
    };
    auto taskDoneFunction = [weakSelf](const CheckResult& result) {
        if (const auto self = weakSelf.lock()) {
            self->reportCheckResult(result);
        }
    };
    if (!android::base::runParallelTask<CheckResult>(mLooper, taskFunction,
                                                     taskDoneFunction)) {
        mIsCheckRunning = false;
        return false;
    }
#endif

    runCheckNonBlocking();

    return true;
}

void AdbLivenessChecker::runCheckBlocking(CheckResult* outResult) const {
    System::ProcessExitCode exitCode;
    const std::string& adbPath = mAdb->adbPath();
    std::string serial = mAdb->serialString();
    if (serial.empty())
        serial = mEmulatorName;

    const std::vector<std::string> adbServerAliveCmd = {adbPath, "devices"};
    if (!System::get()->runCommand(
                adbServerAliveCmd,
                System::RunOptions::WaitForCompletion |
                        System::RunOptions::TerminateOnTimeout,
                mCheckIntervalMs / 3, &exitCode) ||
        exitCode != 0) {
        *outResult = CheckResult::kFailureAdbServerDead;
        return;
    }

    const std::vector<std::string> emulatorAliveCmd = {
            adbPath, "-s", serial, "shell", "exit"};
    if (!System::get()->runCommand(
                emulatorAliveCmd,
                System::RunOptions::WaitForCompletion |
                        System::RunOptions::TerminateOnTimeout,
                mCheckIntervalMs / 3, &exitCode) ||
        exitCode != 0) {
        *outResult = CheckResult::kFailureEmulatorDead;
        return;
    }

    *outResult = AdbLivenessChecker::CheckResult::kOnline;
}

void AdbLivenessChecker::runCheckNonBlocking() {
    if (mDevicesCommand || mShellExitCommand) {
        // already in progress
        return;
    }

    mDevicesCommand =
        mAdb->runAdbCommand(
            {"devices"},
            [this](const android::emulation::OptionalAdbCommandResult& result) {
                if (!result || result->exit_code) {
                    reportCheckResult(CheckResult::kFailureAdbServerDead);
                } else {
                    mShellExitCommand = mAdb->runAdbCommand(
                        {"shell", "exit"},
                        [this](const android::emulation::OptionalAdbCommandResult& result2) {
                            if (!result2 || result2->exit_code) {
                                reportCheckResult(CheckResult::kFailureEmulatorDead);
                            } else {
                                reportCheckResult(CheckResult::kOnline);
                            }

                            mShellExitCommand.reset();
                        },
                        mCheckIntervalMs / 3,
                        false);
                }

                mDevicesCommand.reset();
            },
            mCheckIntervalMs / 3,
            false);
}

void AdbLivenessChecker::reportCheckResult(const CheckResult& result) {
    mIsCheckRunning = false;

    // |mIsOnline| starts off set to false. We set it to true if we successfully
    // talk to the emulator at least once. This allows us to distinguish the
    // cases where adb is missing or emulator doesn't boot at all from cases
    // where emulator boots and then disappears from adb.
    if (!mIsOnline) {
        switch (result) {
            case CheckResult::kFailureAdbServerDead:
                dropMetrics(CheckResult::kFailureNoAdb);
                break;
            case CheckResult::kOnline:
                dropMetrics(result);
                mIsOnline = true;
                break;
            default:
                dropMetrics(CheckResult::kNoResult);
        }
        return;
    }

    if (result == CheckResult::kOnline) {
        mRemainingAttempts = kMaxAttempts;
        dropMetrics(result);
    } else if (--mRemainingAttempts == 0) {
        LOG(VERBOSE) << "Reporting error: " << static_cast<int>(result);
        dropMetrics(result);
    } else {
        LOG(VERBOSE) << "Encountered  error. mRemainingAttempts: "
                     << mRemainingAttempts;
    }
}

void AdbLivenessChecker::dropMetrics(const CheckResult& result) {
    // Only report state if it changes. We start with kNoResult, and we only
    // report kNoResult or kFailureNoAdb until emulator starts.
    if (mCurrentState != result) {
        mCurrentState = result;
        mReporter->report([result](android_studio::AndroidStudioEvent* event) {
            event->mutable_emulator_details()->set_adb_liveness(
                    static_cast<android_studio::EmulatorDetails::
                                        EmulatorAdbLiveness>(
                            static_cast<int>(result)));
        });
    }
}

}  // namespace metrics
}  // namespace android
