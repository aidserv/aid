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

#include "android/emulation/control/AdbInterface.h"

namespace android {
namespace emulation {

class TestAdbInterface final : public AdbInterface {
public:
    explicit TestAdbInterface(android::base::Looper* looper,
                              android::base::StringView adbPath = "adb")
        : mLooper(looper), mAdbPath(adbPath), mSerialString("emulator-0") {}

    virtual bool isAdbVersionCurrent() const override final {
        return mIsAdbVersionCurrent;
    }
    void setCustomAdbPath(const std::string& path) override final {}
    virtual const std::string& detectedAdbPath() const override final {
        return mAdbPath;
    }
    virtual const std::string& adbPath() const override final {
        return mAdbPath;
    }
    virtual void setSerialNumberPort(int port) override final {}
    // Returns the path to emulator name
    virtual const std::string& serialString() const override final {
        return mSerialString;
    }

    void setAdbOptions(android::base::StringView path, bool isVersionCurrent) {
        mIsAdbVersionCurrent = isVersionCurrent;
        mAdbPath = path;
    }

    virtual AdbCommandPtr runAdbCommand(
            const std::vector<std::string>& args,
            std::function<void(const OptionalAdbCommandResult&)>
                    result_callback,
            base::System::Duration timeout_ms,
            bool want_output = true) override final {
        if (mRunCommandCallback) {
            // make sure we allow the testing code to verify the arguments
            mRunCommandCallback(args, result_callback, timeout_ms, want_output);
        }
        // Just do what the real one did - mLooper is the thing that controls
        // the command execution.
        auto command = std::shared_ptr<AdbCommand>(
                new AdbCommand(mLooper, mAdbPath, mSerialString, args, want_output,
                               timeout_ms, result_callback));
        command->start(1);
        return command;
    }

    using RunCommandCallback = std::function<void(
            const std::vector<std::string>&,
            std::function<void(const OptionalAdbCommandResult&)>,
            base::System::Duration,
            bool)>;

    void setRunCommandCallback(RunCommandCallback cb) {
        mRunCommandCallback = cb;
    }

private:
    android::base::Looper* mLooper;
    bool mIsAdbVersionCurrent = false;
    std::string mAdbPath;
    RunCommandCallback mRunCommandCallback;
    std::string mSerialString;
};

}
}
