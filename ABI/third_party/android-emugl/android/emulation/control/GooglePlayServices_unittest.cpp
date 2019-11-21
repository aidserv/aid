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

#include "android/base/Compiler.h"
#include "android/base/StringView.h"
#include "android/base/system/System.h"
#include "android/base/testing/TestSystem.h"

#include <gtest/gtest.h>

#include <string>

using android::base::System;
using android::base::TestSystem;
using android::base::TestTempDir;

using android::emulation::GooglePlayServices;
using std::ofstream;
using std::string;

TEST(GooglePlayServices, parseOutputForVersionNullArgs) {
    string outString;
    std::ifstream f;
    f.open("i-dont-exist.txt");
    EXPECT_FALSE(GooglePlayServices::parseOutputForVersion(f, &outString));
    TestSystem system("/progdir", System::kProgramBitness, "/homedir",
                      "/appdir");
    TestTempDir* dir = system.getTempRoot();
    string outputFile = dir->makeSubPath("output.txt");

    ofstream ofs(outputFile);
    EXPECT_TRUE(ofs.is_open());
    ofs << "versionName=1.0.0";
    ofs.close();

    f.open(outputFile.c_str());
    EXPECT_FALSE(GooglePlayServices::parseOutputForVersion(f, NULL));
}

TEST(GooglePlayServices, parseOutputForVersionNoVersionName) {
    TestSystem system("/progdir", System::kProgramBitness, "/homedir",
                      "/appdir");
    TestTempDir* dir = system.getTempRoot();
    string outputFile = dir->makeSubPath("output.txt");

    ofstream ofs(outputFile);
    EXPECT_TRUE(ofs.is_open());
    ofs << "\tversionName2=v1.0.0\n"
           "\tbad_versionName=v1.1.0\n"
           "worse versionName=v1.2.0\n";
    ofs.close();

    string outString;
    std::ifstream f(outputFile.c_str());
    EXPECT_FALSE(GooglePlayServices::parseOutputForVersion(f, &outString));
}

TEST(GooglePlayServices, parseOutputForVersionSuccess) {
    TestSystem system("/progdir", System::kProgramBitness, "/homedir",
                      "/appdir");
    TestTempDir* dir = system.getTempRoot();
    string outputFile = dir->makeSubPath("output.txt");

    ofstream ofs(outputFile);
    EXPECT_TRUE(ofs.is_open());
    ofs << "versionName=1.0.0";
    ofs.close();

    string outString;
    std::ifstream f(outputFile.c_str());
    EXPECT_TRUE(GooglePlayServices::parseOutputForVersion(f, &outString));
    EXPECT_STREQ(outString.c_str(), "1.0.0");
    f.close();

    ofs.open(outputFile);
    EXPECT_TRUE(ofs.is_open());
    ofs << "package=com.android.vending\n\n"
           "\tversionName=1.0.1\n"
           "\tversionName=0.0.99\n";
    ofs.close();

    f.open(outputFile.c_str());
    EXPECT_TRUE(GooglePlayServices::parseOutputForVersion(f, &outString));
    EXPECT_STREQ(outString.c_str(), "1.0.1");
    f.close();

    ofs.open(outputFile);
    EXPECT_TRUE(ofs.is_open());
    ofs << "package=com.android.vending\n\n"
           "  versionName=1.0.2\n"
           "  versionName=0.0.99\n";
    ofs.close();
    f.open(outputFile.c_str());
    EXPECT_TRUE(GooglePlayServices::parseOutputForVersion(f, &outString));
    EXPECT_STREQ(outString.c_str(), "1.0.2");
    f.close();
}
