// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/qt/qt_path.h"

#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"

#include <string>
#include <vector>

using namespace android::base;

// Get the base directory for libraries and plugins.
static std::string androidQtGetBaseDir(int bitness, const char* emulatorDir) {
    if (!bitness) {
        bitness = System::getProgramBitness();
    }

    System* system = System::get();
    const char* const libBitness = bitness == 64 ? "lib64" : "lib";
    std::vector<std::string> subDirVector;
    subDirVector.push_back(emulatorDir ? emulatorDir : system->getLauncherDirectory());
    subDirVector.push_back(std::string(libBitness));
    subDirVector.push_back(std::string("qt"));
    std::string qtDir = PathUtils::recompose(subDirVector);

    return qtDir;
}

std::string androidQtGetLibraryDir(int bitness, const char* emulatorDir) {
    std::vector<std::string> subDirVector;
    subDirVector.push_back(androidQtGetBaseDir(bitness, emulatorDir));
    subDirVector.push_back(std::string("lib"));
    std::string qtLibDir = PathUtils::recompose(subDirVector);

    return qtLibDir;
}

std::string androidQtGetPluginsDir(int bitness, const char* emulatorDir) {
    System* system = System::get();
    std::string qtPluginsDir =
            system->envGet("ANDROID_QT_QPA_PLATFORM_PLUGIN_PATH");
    if (!qtPluginsDir.empty()) {
        return qtPluginsDir;
    }

    std::vector<std::string> subDirVector;
    subDirVector.push_back(androidQtGetBaseDir(bitness, emulatorDir));
    subDirVector.push_back(std::string("plugins"));
    qtPluginsDir = PathUtils::recompose(subDirVector);

    return qtPluginsDir;
}
