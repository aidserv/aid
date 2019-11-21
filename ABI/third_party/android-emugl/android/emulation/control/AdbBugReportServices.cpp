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

#include "android/emulation/control/AdbBugReportServices.h"

#include "android/base/StringFormat.h"
#include "android/base/Uuid.h"
#include "android/base/files/PathUtils.h"
#include "android/globals.h"

#include <ctime>
#include <fstream>
#include <iterator>
#include <string>

namespace android {
namespace emulation {

using android::base::PathUtils;
using android::base::StringView;
using android::base::System;
using android::base::StringFormat;
using android::base::Uuid;

const System::Duration AdbBugReportServices::kAdbCommandTimeoutMs =
        System::kInfinite;

AdbBugReportServices::~AdbBugReportServices() {
    if (mAdbBugReportCommand) {
        mAdbBugReportCommand->cancel();
    }
    if (mAdbLogcatCommand) {
        mAdbLogcatCommand->cancel();
    }
}

void AdbBugReportServices::generateBugReport(StringView outputDirectoryPath,
                                             ResultCallback resultCallback) {
    if (isBugReportInFlight()) {
        resultCallback(Result::OperationInProgress, nullptr);
        return;
    }

    if (!System::get()->pathIsDir(outputDirectoryPath) ||
        !System::get()->pathCanWrite(outputDirectoryPath)) {
        resultCallback(Result::SaveLocationInvalid, nullptr);
        return;
    }

    auto filePath =
            PathUtils::join(outputDirectoryPath, generateUniqueBugreportName());

    // In reference to
    // platform/frameworks/native/cmds/dumpstate/bugreport-format.md
    // Issue different command args given the API level

    int apiLevel = avdInfo_getApiLevel(android_avdInfo);
    if (apiLevel != kDefaultUnknownAPILevel && apiLevel > 23) {
        filePath.append(".zip");
        mAdbBugReportCommand = mAdb->runAdbCommand(
                {"bugreport", filePath},
                [this, resultCallback,
                 filePath](const OptionalAdbCommandResult& result) {
                    if (!result || result->exit_code) {
                        resultCallback(Result::GenerationFailed, nullptr);
                    } else {
                        resultCallback(Result::Success, filePath);
                    }
                    mAdbBugReportCommand.reset();
                },
                kAdbCommandTimeoutMs, false);
    } else {
        filePath.append(".txt");
        mAdbBugReportCommand = mAdb->runAdbCommand(
                {"bugreport"},
                [this, resultCallback,
                 filePath](const OptionalAdbCommandResult& result) {
                    if (!result || result->exit_code || !result->output) {
                        resultCallback(Result::GenerationFailed, nullptr);
                    } else {
                        std::ofstream outFile(
                                filePath.c_str(),
                                std::ios_base::out | std::ios_base::trunc);
                        if (!outFile.is_open() || !outFile.good()) {
                            resultCallback(Result::GenerationFailed, nullptr);
                        } else {
                            std::filebuf* pbuf = result->output->rdbuf();
                            outFile << pbuf;
                            outFile.close();
                            resultCallback(Result::Success, filePath);
                        }
                    }
                    mAdbBugReportCommand.reset();
                },
                kAdbCommandTimeoutMs, true);
    }
}

void AdbBugReportServices::generateAdbLogcatInMemory(
        ResultOutputCallback resultCallback) {
    if (mAdbLogcatCommand) {
        resultCallback(Result::OperationInProgress, nullptr);
        return;
    }
    // After apiLevel 19, buffer "all" become available
    int apiLevel = avdInfo_getApiLevel(android_avdInfo);
    mAdbLogcatCommand = mAdb->runAdbCommand(
            (apiLevel != kDefaultUnknownAPILevel && apiLevel > 19)
                    ? std::vector<std::string>{"logcat", "-b", "all", "-d"}
                    : std::vector<std::string>{"logcat", "-b", "events", "-b",
                                               "main", "-b", "radio", "-b",
                                               "system", "-d"},
            [this, resultCallback](const OptionalAdbCommandResult& result) {
                if (!result || result->exit_code || !result->output) {
                    resultCallback(Result::GenerationFailed, nullptr);
                } else {
                    std::string s(
                            std::istreambuf_iterator<char>(*result->output),
                            {});
                    resultCallback(Result::Success, s);
                }
                mAdbLogcatCommand.reset();
            },
            kAdbCommandTimeoutMs, true);
}

std::string AdbBugReportServices::generateUniqueBugreportName() {
    const char* deviceName = avdInfo_getName(android_avdInfo);
    time_t now = System::get()->getUnixTime();
    char date[80];
    strftime(date, sizeof(date), "%Y-%m-%d-%H-%M-%S", localtime(&now));
    return StringFormat("bugreport-%s-%s-%s",
                        deviceName ? deviceName : "UNKNOWN_DEVICE", date,
                        Uuid::generate().toString());
}

}  // namespace emulation
}  // namespace android
