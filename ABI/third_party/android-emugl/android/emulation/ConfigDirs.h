// Copyright 2015 The Android Open Source Project
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

#include "android/base/StringView.h"

#include <string>

namespace android {

// Helper struct to hold static methods related to misc. configuration
// directories used by the emulator.
struct ConfigDirs {
    // Return the user-specific directory containing Android-related
    // configuration files. The default location can be overriden:
    //
    // - If ANDROID_EMULATOR_HOME is defined in the environment, its
    //   value is returned by this function.
    //
    // - Otherwise, if ANDROID_SDK_HOME is defined in the environment,
    //   this function returns one of its sub-directories.
    //
    // - Otherwise, this returns a subdirectory of the user's home dir.
    static std::string getUserDirectory();

    // Return the root path containing all AVD sub-directories.
    // More specifically:
    //
    // - If $ANDROID_AVD_HOME points to a directory, it is returned.
    //
    // - Else if $ANDROID_SDK_HOME/.android/avd is a valid root, it is returned.
    //
    // - Else if $ANDROID_SDK_HOME is defined but does not lead to a valid root
    //   - $USER_HOME/.android/avd is returned if it is valid, or
    //   - $HOME/.android/avd is returned if it is valid
    //
    // - Otherwise, a sub-directory named 'avd' of getUserDirectory()'s
    //   output is returned.
    static std::string getAvdRootDirectory();

    // Returns the path to the root of the android sdk by checking
    // - ANDROID_HOME, then
    // - ANDROID_SDK_ROOT
    static std::string getSdkRootDirectoryByEnv();

    // Returns the path to the root of the android sdk by inferring it from the
    // path of the running emulator binary.
    static std::string getSdkRootDirectoryByPath();

    // Returns the path to the root of the android sdk.
    // - If ANDROID_SDK_ROOT is defined in the environment and the path exists,
    //   its value is returned.
    //
    // - Otherwise, Sdk root is inferred from the path of the running emulator
    //   binary.
    static std::string getSdkRootDirectory();

private:
    // Check if the specified path is a valid AVD root path.
    // It is considered valid if it has an 'avd' subdirectory
    static bool isValidAvdRoot(android::base::StringView avdPath);

    // Check if the specified path is a valid SDK root path.
    // It is considered valid if it has a 'platforms' subdirectory
    // and a 'platform-tools' subdirectory.
    static bool isValidSdkRoot(android::base::StringView rootPath);
};

}  // namespace android
