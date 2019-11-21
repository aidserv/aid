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

#include "android/emulation/ComponentVersion.h"

#include "android/base/files/PathUtils.h"
#include "android/base/testing/TestSystem.h"

#include <gtest/gtest.h>

#include <fstream>

using android::base::PathUtils;
using android::base::StringView;
using android::base::System;
using android::base::TestSystem;
using android::base::TestTempDir;
using android::base::Version;
using android::SdkComponentType;

TEST(ComponentVersion, ProperPlatformToolsVersion) {
    TestSystem system("/progdir", System::kProgramBitness, "/homedir",
                      "/appdir");
    TestTempDir* dir = system.getTempRoot();
    ASSERT_TRUE(dir->makeSubDir("Sdk"));
    ASSERT_TRUE(dir->makeSubDir("Sdk/platform-tools"));
    std::string output_file =
            dir->makeSubPath("Sdk/platform-tools/source.properties");
    std::ofstream ofs(output_file);
    ASSERT_TRUE(ofs.is_open());
    ofs << "### Comment\nArchive.HostOs=linux\nPkg.License=\\nNoliense\n"
           "Pkg.LicenseRef=android-sdk-license\nPkg.Revision=23.1.0\n"
           "Pkg.SourceUrl=https\\://dl.google.com/android/repository/"
           "repository-12.xml\n";
    ofs.close();
    std::string sdkRoot = PathUtils::join(dir->path(), "Sdk");
    Version version = android::getCurrentSdkVersion(
            sdkRoot, SdkComponentType::PlatformTools);
    EXPECT_EQ("23.1.0", version.toString());
}

TEST(ComponentVersion, ProperToolsVersion) {
    TestSystem system("/progdir", System::kProgramBitness, "/homedir",
                      "/appdir");
    TestTempDir* dir = system.getTempRoot();
    ASSERT_TRUE(dir->makeSubDir("Sdk"));
    ASSERT_TRUE(dir->makeSubDir("Sdk/tools"));
    std::string output_file = dir->makeSubPath("Sdk/tools/source.properties");
    std::ofstream ofs(output_file);
    ASSERT_TRUE(ofs.is_open());
    ofs << "### Comment\nArchive.HostOs=linux\nPkg.License=\\nNoliense\n"
           "Pkg.LicenseRef=android-sdk-license\nPkg.Revision=23.0.1\n"
           "Pkg.SourceUrl=https\\://dl.google.com/android/repository/"
           "repository-12.xml\n";
    ofs.close();
    std::string sdkRoot = PathUtils::join(dir->path(), "Sdk");
    Version version =
            android::getCurrentSdkVersion(sdkRoot, SdkComponentType::Tools);
    EXPECT_EQ("23.0.1", version.toString());
}

TEST(ComponentVersion, InvalidToolsVersion) {
    TestSystem system("/progdir", System::kProgramBitness, "/homedir",
                      "/appdir");
    TestTempDir* dir = system.getTempRoot();
    ASSERT_TRUE(dir->makeSubDir("Sdk"));
    ASSERT_TRUE(dir->makeSubDir("Sdk/tools"));
    std::string output_file = dir->makeSubPath("Sdk/tools/source.properties");
    std::ofstream ofs(output_file);
    ASSERT_TRUE(ofs.is_open());
    ofs << "### Comment\nArchive.HostOs=linux\nPkg.License=\\nNoliense\n"
           "Pkg.LicenseRef=android-sdk-license\nPkg.Revision=invalid version\n"
           "Pkg.SourceUrl=https\\://dl.google.com/android/repository/"
           "repository-12.xml\n";
    ofs.close();
    std::string sdkRoot = PathUtils::join(dir->path(), "Sdk");
    Version version =
            android::getCurrentSdkVersion(sdkRoot, SdkComponentType::Tools);
    EXPECT_EQ("invalid", version.toString());
}
