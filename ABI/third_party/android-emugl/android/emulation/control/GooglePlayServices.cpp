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

#include "android/emulation/control/GooglePlayServices.h"

namespace android {
namespace emulation {

using android::base::StringView;
using android::base::System;
using std::string;

// static
const System::Duration GooglePlayServices::kAdbCommandTimeoutMs = 5000;
constexpr StringView GooglePlayServices::kPlayServicesPkgName;

GooglePlayServices::~GooglePlayServices() {
    if (mServicesPageCommand) {
        mServicesPageCommand->cancel();
    }
    if (mServicesVersionCommand) {
        mServicesVersionCommand->cancel();
    }
    if (mGetpropCommand) {
        mGetpropCommand->cancel();
    }
}

// static
bool GooglePlayServices::parseOutputForVersion(std::istream& stream,
                                               string* outString) {
    // Only accept output in the form of
    //
    //     <W>versionName=<version>
    //
    // Where <W> can be any number/kind of whitespace characters.

    const StringView keystr = "versionName=";
    string line;

    // "dumpsys package <pkgname>" may return more than one version
    // name, depending on if there were any updates installed. In
    // these cases, we only need the latest version name, which is
    // on the first line returned. If there is no output, or no
    // value for versionName, we can assume that this package is
    // not installed.

    if (!stream || !outString) {
        return false;
    }

    while (getline(stream, line)) {
        auto keyPos = line.find(keystr);
        if (keyPos != string::npos && keyPos + keystr.size() < line.length()) {
            // Make sure everything before is only whitespace.
            bool isValid = true;
            for (size_t i = 0; i < keyPos; ++i) {
                if (!std::isspace(static_cast<unsigned char>(line[i]))) {
                    isValid = false;
                    break;
                }
            }
            if (isValid) {
                *outString =
                        line.substr(keyPos + keystr.size(),
                                    line.length() - (keyPos + keystr.size()));
                return true;
            }
        }
    }
    return false;
}

void GooglePlayServices::getSystemProperty(
        android::base::StringView sysProp,
        ResultOutputCallback resultCallback) {
    if (!mAdb) {
        return;
    }

    if (mGetpropCommand) {
        resultCallback(Result::OperationInProgress, "");
        return;
    }
    std::vector<std::string> getpropCommand{"shell", "getprop", sysProp};
    mGetpropCommand = mAdb->runAdbCommand(
            getpropCommand,
            [resultCallback, this](const OptionalAdbCommandResult& result) {
                string line;
                if (result && result->output &&
                    getline(*(result->output), line)) {
                    resultCallback(Result::Success, line);
                } else {
                    resultCallback(Result::UnknownError, "");
                }
                mGetpropCommand.reset();
            },
            kAdbCommandTimeoutMs, true);
}

void GooglePlayServices::showPlayServicesPage(ResultCallback resultCallback) {
    if (!mAdb) {
        return;
    }

    if (mServicesPageCommand) {
        resultCallback(Result::OperationInProgress);
        return;
    }
    mServicesPageCommand = mAdb->runAdbCommand(
            {"shell", "am start", "-a", "android.intent.action.VIEW", "-d",
             "'market://details?id=" + std::string(kPlayServicesPkgName) + "'"},
            [this, resultCallback](const OptionalAdbCommandResult& result) {
                resultCallback((!result || result->exit_code)
                                       ? Result::UnknownError
                                       : Result::Success);
                mServicesPageCommand.reset();
            },
            kAdbCommandTimeoutMs, false);
}

void GooglePlayServices::getPlayServicesVersion(
        ResultOutputCallback resultCallback) {
    if (!mAdb) {
        return;
    }

    if (mServicesVersionCommand) {
        resultCallback(Result::OperationInProgress, "");
        return;
    }
    mServicesVersionCommand = mAdb->runAdbCommand(
            {"shell", "dumpsys", "package", kPlayServicesPkgName},
            [this, resultCallback](const OptionalAdbCommandResult& result) {
                if (!result || result->exit_code) {
                    resultCallback(Result::UnknownError, "");
                } else {
                    string outString;
                    const bool parseResult = parseOutputForVersion(
                            *(result->output), &outString);
                    resultCallback(parseResult ? Result::Success
                                               : Result::AppNotInstalled,
                                   outString);
                }
                mServicesVersionCommand.reset();
            },
            kAdbCommandTimeoutMs, true);
}

}  // namespace emulation
}  // namespace android
