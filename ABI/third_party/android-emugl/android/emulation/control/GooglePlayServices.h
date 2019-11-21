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

#include <functional>
#include <string>

namespace android {
namespace emulation {

class GooglePlayServices {
public:
    enum class Result {
        Success,
        OperationInProgress,
        AppNotInstalled,
        UnknownError
    };

    using ResultCallback = std::function<void(Result result)>;
    using ResultOutputCallback =
            std::function<void(Result result,
                               android::base::StringView outString)>;

    explicit GooglePlayServices(AdbInterface* adb) : mAdb(adb) {}
    ~GooglePlayServices();

    // Opens the Play Services app details page in the play store.
    void showPlayServicesPage(ResultCallback resultCallback);
    // Get a string of the latest version number of play services.
    void getPlayServicesVersion(ResultOutputCallback resultCallback);
    // Get a system property.
    void getSystemProperty(android::base::StringView sysProp,
                           ResultOutputCallback resultCallback);

    // Parses the contents of |output| stream as if it was the output from
    // running "adb shell dumpsys <pkgname>" to get the
    // latest version name of the package. Returns true if the output has
    // a line in the form of "versionName=<pkgVersion>", false otherwise.
    // There may be leading spaces in the line.
    static bool parseOutputForVersion(std::istream& output,
                                      std::string* outErrorString);

private:
    static const android::base::System::Duration kAdbCommandTimeoutMs;
    static constexpr android::base::StringView kPlayServicesPkgName =
            "com.google.android.gms";

    AdbInterface* mAdb;
    AdbCommandPtr mGetpropCommand;
    AdbCommandPtr mServicesPageCommand;
    AdbCommandPtr mServicesVersionCommand;
    const ResultCallback mResultCallback;

    DISALLOW_COPY_AND_ASSIGN(GooglePlayServices);
};

}  // namespace emulation
}  // namespace android
