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

#include "android/emulation/control/AdbInterface.h"

#include "android/base/StringView.h"
#include "android/base/Uuid.h"
#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"
#include "android/emulation/ComponentVersion.h"
#include "android/emulation/ConfigDirs.h"
#include "android/utils/debug.h"

#include <cstdio>
#include <fstream>
#include <string>

using namespace android::base;

namespace android {
namespace emulation {

class AdbInterfaceImpl final : public AdbInterface {
public:
    explicit AdbInterfaceImpl(android::base::Looper* looper);

    // Returns true is the ADB version is fresh enough.
    bool isAdbVersionCurrent() const override final { return mAdbVersionCurrent; }

    // Setup a custom adb path.
    void setCustomAdbPath(const std::string& path) override final {
        mCustomAdbPath = path;
    }

    // Returns the automatically detected path to adb
    const std::string& detectedAdbPath() const override final {
        return mAutoAdbPath;
    }

    // Returns the path to adb binary
    const std::string& adbPath() const override final {
        return mCustomAdbPath.empty() ? mAutoAdbPath : mCustomAdbPath;
    }

    // Setup the port this interface is connected to
    virtual void setSerialNumberPort(int port) override final {
        mSerialString = std::string("emulator-") + std::to_string(port);
    }

    // Returns the path to emulator name
    const std::string& serialString() const override final {
        return mSerialString;
    }

    // Runs an adb command asynchronously.
    // |args| - the arguments to pass to adb, i.e. "shell dumpsys battery"
    // |result_callback| - the callback function that will be invoked on the
    // calling
    //                     thread after the command completes.
    // |timeout_ms| - how long to wait for the command to complete, in
    // milliseconds.
    // |want_output| - if set to true, the argument passed to the callback will
    // contain the
    //                 output of the command.
    AdbCommandPtr runAdbCommand(
            const std::vector<std::string>& args,
            std::function<void(const OptionalAdbCommandResult&)>
                    result_callback,
            base::System::Duration timeout_ms,
            bool want_output = true) override final;

private:
    android::base::Looper* mLooper;
    std::string mAutoAdbPath;
    std::string mCustomAdbPath;
    std::string mSerialString;
    bool mAdbVersionCurrent;
};

std::unique_ptr<AdbInterface> AdbInterface::create(android::base::Looper* looper) {
    return std::unique_ptr<AdbInterface>{new AdbInterfaceImpl(looper)};
}

// Helper function, checks if the version of adb in the given SDK is
// fresh enough.
static bool checkAdbVersion(const std::string& sdk_root_directory,
                            const std::string& adb_path) {
    static const int kMinAdbVersionMajor = 23;
    static const int kMinAdbVersionMinor = 1;
    static const Version kMinAdbVersion(kMinAdbVersionMajor,
                                        kMinAdbVersionMinor, 0);
    if (sdk_root_directory.empty()) {
        return false;
    }

    if (!System::get()->pathCanExec(adb_path)) {
        return false;
    }

    Version version = android::getCurrentSdkVersion(
            sdk_root_directory, android::SdkComponentType::PlatformTools);

    if (version.isValid()) {
        return !(version < kMinAdbVersion);

    } else {
        // If the version is invalid, assume the tools directory is broken in
        // some way, and updating should fix the problem.
        return false;
    }
}

AdbInterfaceImpl::AdbInterfaceImpl(android::base::Looper* looper)
    : mLooper(looper), mAdbVersionCurrent(false) {
    // First try finding ADB by the environment variable.
    auto sdk_root_by_env = android::ConfigDirs::getSdkRootDirectoryByEnv();
    if (!sdk_root_by_env.empty()) {
        // If ANDROID_SDK_ROOT is defined, the user most likely wanted to use
        // that ADB. Store it for later - if the second potential ADB path
        // also fails, we'll warn the user about this one.
        auto adb_path = PathUtils::join(sdk_root_by_env, "platform-tools",
                                        PathUtils::toExecutableName("adb"));
        if (checkAdbVersion(sdk_root_by_env, adb_path)) {
            mAutoAdbPath = adb_path;
            mAdbVersionCurrent = true;
            return;
        }
    }

    // If the first path was non-existent or a bad version, try to infer the
    // path based on the emulator executable location.
    auto sdk_root_by_path = android::ConfigDirs::getSdkRootDirectoryByPath();
    if (sdk_root_by_path != sdk_root_by_env && !sdk_root_by_path.empty()) {
        auto adb_path = PathUtils::join(sdk_root_by_path, "platform-tools",
                                        PathUtils::toExecutableName("adb"));
        if (checkAdbVersion(sdk_root_by_path, adb_path)) {
            mAutoAdbPath = adb_path;
            mAdbVersionCurrent = true;
            return;
        }
    }

    // TODO(zyy): check if there's an adb binary on %PATH% and use that as a
    //  last line of defense.

    // If no ADB has been found at this point, an error message will warn the
    // user and direct them to the custom adb path setting.
}

AdbCommandPtr AdbInterfaceImpl::runAdbCommand(
        const std::vector<std::string>& args,
        std::function<void(const OptionalAdbCommandResult&)> result_callback,
        base::System::Duration timeout_ms,
        bool want_output) {
    auto command = std::shared_ptr<AdbCommand>(new AdbCommand(
            mLooper, mCustomAdbPath.empty() ? mAutoAdbPath : mCustomAdbPath,
            mSerialString, args, want_output, timeout_ms, result_callback));
    command->start();
    return command;
}

AdbCommand::AdbCommand(android::base::Looper* looper,
                       const std::string& adb_path,
                       const std::string& serial_string,
                       const std::vector<std::string>& command,
                       bool want_output,
                       base::System::Duration timeout,
                       AdbCommand::ResultCallback callback)
    : mLooper(looper),
      mResultCallback(callback),
      mOutputFilePath(PathUtils::join(
              System::get()->getTempDir(),
              std::string("adbcommand").append(Uuid::generate().toString()))),
      mWantOutput(want_output),
      mTimeout(timeout),
      mFinished(false) {
    mCommand.push_back(adb_path);

    // TODO: when run headless, the serial string won't be properly
    // initialized, so make a best attempt by using -e. This should be updated
    // when the headless emulator is given an AdbInterface reference.
    if (serial_string.empty()) {
        mCommand.push_back("-e");
    } else {
        mCommand.push_back("-s");
        mCommand.push_back(serial_string);
    }

    mCommand.insert(mCommand.end(), command.begin(), command.end());
}

void AdbCommand::start(int checkTimeoutMs) {
    if (!mTask && !mFinished) {
        auto shared = shared_from_this();
        mTask.reset(new ParallelTask<OptionalAdbCommandResult>(
                mLooper,
                [shared](OptionalAdbCommandResult* result) {
                    shared->taskFunction(result);
                },
                [shared](const OptionalAdbCommandResult& result) {
                    shared->taskDoneFunction(result);
                },
                checkTimeoutMs));
        mTask->start();
    }
}

void AdbCommand::taskDoneFunction(const OptionalAdbCommandResult& result) {
    if (!mCancelled) {
        mResultCallback(result);
    }
    mFinished = true;
    // This may invalidate this object and clean it up.
    // DO NOT reference any internal state from this class after this
    // point.
    mTask.reset();
}

void AdbCommand::taskFunction(OptionalAdbCommandResult* result) {
    if (mCommand.empty() || mCommand.front().empty()) {
        *result = {};
        return;
    }

    RunOptions output_flag = mWantOutput ? System::RunOptions::DumpOutputToFile
                                         : System::RunOptions::HideAllOutput;
    RunOptions run_flags = System::RunOptions::WaitForCompletion |
                           System::RunOptions::TerminateOnTimeout | output_flag;
    System::Pid pid;
    android::base::System::ProcessExitCode exit_code;

    bool command_ran = System::get()->runCommand(
            mCommand, run_flags, mTimeout, &exit_code, &pid, mOutputFilePath);

    if (command_ran) {
        *result = android::base::makeOptional<AdbCommandResult>(
                {exit_code,
                 mWantOutput ? std::unique_ptr<std::ifstream>(new std::ifstream(
                                       mOutputFilePath.c_str()))
                             : std::unique_ptr<std::ifstream>()});
    }
}

}
}
