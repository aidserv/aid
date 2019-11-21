// Copyright (C) 2017 The Android Open Source Project
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
#include "android/base/system/System.h"
#include "android/emulation/control/AdbInterface.h"

namespace android {
namespace emulation {

// This class provides the interface to invoke adb bugreport and adb logcat
// in the guest system for different API Levels. The underlying AdbInterface
// instance must be unique to avoid conflicts.
class AdbBugReportServices {
public:
    enum class Result {
        Success,
        OperationInProgress,
        GenerationFailed,
        SaveLocationInvalid,
        UnknownError
    };

    using ResultCallback =
            std::function<void(Result result,
                               android::base::StringView filePath)>;
    using ResultOutputCallback =
            std::function<void(Result result,
                               android::base::StringView output)>;
    explicit AdbBugReportServices(AdbInterface* adb) : mAdb(adb) {}
    ~AdbBugReportServices();

    // Generate an adb bugreport file saved at file path
    // |outputDirectoryPath|, this function might take minutes to finish
    void generateBugReport(android::base::StringView outputDirectoryPath,
                           ResultCallback resultCallback);
    // Returns true if the adb bugreport command is currently in the process of execution.
    bool isBugReportInFlight() const {
        return mAdbBugReportCommand != nullptr;
    };

    void generateAdbLogcatInMemory(ResultOutputCallback resultCallback);

    // Returns true if the adb logcat command is currently in the process of
    // execution.
    bool isLogcatInFlight() const { return mAdbLogcatCommand != nullptr; }

    // bug report folder will be in the format of
    // bugreport-[deviceName]-[%Y-%m-%d%H:%M:%S]-[Uuid]
    static std::string generateUniqueBugreportName();

private:
    static const android::base::System::Duration kAdbCommandTimeoutMs;
    static const int kDefaultUnknownAPILevel = 1000;
    AdbInterface* mAdb;
    AdbCommandPtr mAdbBugReportCommand;
    AdbCommandPtr mAdbLogcatCommand;

    DISALLOW_COPY_AND_ASSIGN(AdbBugReportServices);
};

}  // namespace emulation
}  // namespace android
