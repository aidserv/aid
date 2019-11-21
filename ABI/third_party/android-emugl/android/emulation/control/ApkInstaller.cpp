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

#include "android/emulation/control/ApkInstaller.h"

#include "android/base/StringFormat.h"
#include "android/base/Uuid.h"
#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"
#include "android/base/threads/Async.h"

#include <fstream>
#include <iostream>
#include <sstream>

namespace android {
namespace emulation {

using android::base::Looper;
using android::base::ParallelTask;
using android::base::PathUtils;
using android::base::StringView;
using android::base::System;
using android::base::Uuid;
using std::string;
using std::vector;

// static
// TODO(birenbaum): what's an appropriate number here? We don't want to go
// forever but some apps are going to be super big and take a long time.
// This is especially troubling since we don't have a reliable way of killing
// the new process besides using the out PID argument and killing that process
// when the user presses cancel (yikes).
const System::Duration ApkInstaller::kInstallTimeoutMs = System::kInfinite;
const char ApkInstaller::kDefaultErrorString[] = "Could not parse error string";

ApkInstaller::ApkInstaller(AdbInterface* adb) : mAdb(adb) {}
ApkInstaller::~ApkInstaller() {
    if (mInstallCommand) {
        mInstallCommand->cancel();
    }
}

// static
bool ApkInstaller::parseOutputForFailure(std::ifstream& stream,
                                         string* outErrorString) {
    // "adb install" does not return a helpful exit status, so instead we parse
    // the output of the process looking for:
    // - "adb: error: failed to copy" in the case that the apk could not be
    //   copied because the device is full
    // - "Failure [<some error code>]", in the case that the apk could not be
    //   installed because of a specific error

    if (!stream) {
        *outErrorString = kDefaultErrorString;
        return false;
    }

    string line;
    while (getline(stream, line)) {
        if (!line.compare(0, 7, "Failure") ||
            !line.compare(0, 6, "Failed")) {
            auto openBrace = line.find("[");
            auto closeBrace = line.find("]", openBrace + 1);
            if (openBrace != string::npos && closeBrace != string::npos) {
                *outErrorString =
                        line.substr(openBrace + 1, closeBrace - openBrace - 1);
            } else {
                *outErrorString = kDefaultErrorString;
            }
            return false;
        } else if (!line.compare(0, 26, "adb: error: failed to copy")) {
            *outErrorString = "No space left on device";
            return false;
        }
    }

    return true;
}

AdbCommandPtr ApkInstaller::install(
        android::base::StringView apkFilePath,
        ApkInstaller::ResultCallback resultCallback) {
    if (!base::System::get()->pathCanRead(apkFilePath)) {
        resultCallback(Result::kApkPermissionsError, "");
        return nullptr;
    }
    std::vector<std::string> installCommand{"install", "-r", "-t", apkFilePath};
    mInstallCommand = mAdb->runAdbCommand(
            installCommand,
            [resultCallback, this](const OptionalAdbCommandResult& result) {
                if (!result) {
                    resultCallback(Result::kAdbConnectionFailed, "");
                } else {
                    std::string errorString;
                    const bool parseResult = parseOutputForFailure(
                            *(result->output), &errorString);
                    resultCallback(parseResult ? Result::kSuccess
                                               : Result::kInstallFailed,
                                   errorString);
                }
                mInstallCommand.reset();
            },
            kInstallTimeoutMs, true);
    return mInstallCommand;
}

}  // namespace emulation
}  // namespace android
