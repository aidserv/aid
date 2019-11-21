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

#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

// Append the application's directory to a bounded |buffer| that stops at
// |buffend|, and return the new position.
extern char* bufprint_app_dir(char* buffer, char* buffend);

// Append the root path containing all AVD sub-directories to a bounded
// |buffer| that stops at |buffend| and return new position. The default
// location can be overriden by defining ANDROID_AVD_HOME in the environment.
extern char* bufprint_avd_home_path(char* buffer, char* buffend);

// Append the user-specific emulator configuration directory to a bounded
// |buffer| that stops at |buffend| and return the new position. The default
// location can be overriden by defining ANDROID_EMULATOR_HOME in the
// environment. Otherwise, a sub-directory of $HOME is used, unless
// ANDROID_SDK_HOME is also defined.
extern char* bufprint_config_path(char* buffer, char* buffend);

// Append the name or a file |suffix| relative to the configuration
// directory (see bufprint_config_path) to the bounded |buffer| that stops
// at |buffend|, and return the new position.
extern char* bufprint_config_file(char* buffer,
                                  char* buffend,
                                  const char* suffix);

ANDROID_END_HEADER
