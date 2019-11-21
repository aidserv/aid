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

#include "android/emulation/bufprint_config_dirs.h"

#include "android/base/system/System.h"
#include "android/emulation/ConfigDirs.h"
#include "android/utils/bufprint.h"
#include "android/utils/debug.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

using android::base::System;
using android::ConfigDirs;

// Implementation of bufprintf_xxx() functions that rely on
// android::base::System to make them mockable during unit-testing.

char* bufprint_app_dir(char* buff, char* end) {
    std::string appDir = System::get()->getLauncherDirectory();
    return bufprint(buff, end, "%s", appDir.c_str());
}

char* bufprint_avd_home_path(char* buff, char* end) {
    return bufprint(buff, end, ConfigDirs::getAvdRootDirectory().c_str());
}

char* bufprint_config_path(char* buff, char* end) {
    // Name of the Android configuration directory under $HOME.
    return bufprint(buff, end, "%s", ConfigDirs::getUserDirectory().c_str());
}

char* bufprint_config_file(char* buff, char* end, const char* suffix) {
    char* p = bufprint_config_path(buff, end);
    p = bufprint(p, end, "%c%s", System::kDirSeparator, suffix);
    return p;
}
