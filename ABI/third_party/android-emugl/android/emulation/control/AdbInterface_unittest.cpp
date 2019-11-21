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

#include "android/emulation/control/AdbInterface.h"

#include "android/base/files/PathUtils.h"
#include "android/base/testing/TestSystem.h"
#include "android/emulation/ConfigDirs.h"

#include <gtest/gtest.h>

#include <fstream>

using android::base::PathUtils;
using android::base::System;
using android::base::TestSystem;
using android::base::TestTempDir;
using android::emulation::AdbInterface;

TEST(AdbInterface, create) {
    EXPECT_NE(nullptr, AdbInterface::create(nullptr).get());
}

TEST(AdbInterface, freshAdbVersion) {
    TestSystem system("/progdir", System::kProgramBitness, "/homedir",
                      "/appdir");
    TestTempDir* dir = system.getTempRoot();
    ASSERT_TRUE(dir->makeSubDir("Sdk"));
    ASSERT_TRUE(dir->makeSubDir("Sdk/platforms"));
    ASSERT_TRUE(dir->makeSubDir("Sdk/platform-tools"));
    ASSERT_TRUE(
            dir->makeSubFile(PathUtils::join("Sdk", "platform-tools",
                                             PathUtils::toExecutableName("adb"))
                                     .c_str()));
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
    system.envSet("ANDROID_SDK_ROOT", sdkRoot);
    auto adb = AdbInterface::create(nullptr);
    EXPECT_TRUE(adb->isAdbVersionCurrent());
    std::string expectedAdbPath = PathUtils::join(
            sdkRoot, "platform-tools", PathUtils::toExecutableName("adb"));
    EXPECT_EQ(expectedAdbPath, adb->detectedAdbPath());
}

TEST(AdbInterface, freshAdbVersionNoMinor) {
    TestSystem system("/progdir", System::kProgramBitness, "/homedir",
                      "/appdir");
    TestTempDir* dir = system.getTempRoot();
    ASSERT_TRUE(dir->makeSubDir("Sdk"));
    ASSERT_TRUE(dir->makeSubDir("Sdk/platforms"));
    ASSERT_TRUE(dir->makeSubDir("Sdk/platform-tools"));
    ASSERT_TRUE(
            dir->makeSubFile(PathUtils::join("Sdk", "platform-tools",
                                             PathUtils::toExecutableName("adb"))
                                     .c_str()));
    std::string output_file =
            dir->makeSubPath("Sdk/platform-tools/source.properties");
    std::ofstream ofs(output_file);
    ASSERT_TRUE(ofs.is_open());
    ofs << "### Comment\nArchive.HostOs=linux\nPkg.License=\\nNoliense\n"
           "Pkg.LicenseRef=android-sdk-license\nPkg.Revision=24 rc1\n"
           "Pkg.SourceUrl=https\\://dl.google.com/android/repository/"
           "repository-12.xml\n";
    ofs.close();
    std::string sdkRoot = PathUtils::join(dir->path(), "Sdk");
    system.envSet("ANDROID_SDK_ROOT", sdkRoot);
    auto adb = AdbInterface::create(nullptr);
    EXPECT_TRUE(adb->isAdbVersionCurrent());
    std::string expectedAdbPath = PathUtils::join(
            sdkRoot, "platform-tools", PathUtils::toExecutableName("adb"));
    EXPECT_EQ(expectedAdbPath, adb->detectedAdbPath());
}

TEST(AdbInterface, staleAdbMinorVersion) {
    TestSystem system("/progdir", System::kProgramBitness, "/homedir",
                      "/appdir");
    TestTempDir* dir = system.getTempRoot();
    ASSERT_TRUE(dir->makeSubDir("Sdk"));
    ASSERT_TRUE(dir->makeSubDir("Sdk/platform-tools"));
    ASSERT_TRUE(
            dir->makeSubFile(PathUtils::join("Sdk", "platform-tools",
                                             PathUtils::toExecutableName("adb"))
                                     .c_str()));
    std::string output_file =
            dir->makeSubPath("Sdk/platform-tools/source.properties");
    std::ofstream ofs(output_file);
    ASSERT_TRUE(ofs.is_open());
    ofs << "### Comment\nArchive.HostOs=linux\nPkg.License=\\nNoliense\n"
           "Pkg.LicenseRef=android-sdk-license\nPkg.Revision=23.0.0\n"
           "Pkg.SourceUrl=https\\://dl.google.com/android/repository/"
           "repository-12.xml\n";
    ofs.close();
    std::string sdkRoot = PathUtils::join(dir->path(), "Sdk");
    system.envSet("ANDROID_SDK_ROOT", sdkRoot);
    auto adb = AdbInterface::create(nullptr);
    EXPECT_FALSE(adb->isAdbVersionCurrent());
    EXPECT_EQ("", adb->detectedAdbPath());
}

TEST(AdbInterface, staleAdbMajorVersion) {
    TestSystem system("/progdir", System::kProgramBitness, "/homedir",
                      "/appdir");
    TestTempDir* dir = system.getTempRoot();
    ASSERT_TRUE(dir->makeSubDir("Sdk"));
    ASSERT_TRUE(dir->makeSubDir("Sdk/platform-tools"));
    ASSERT_TRUE(
            dir->makeSubFile(PathUtils::join("Sdk", "platform-tools",
                                             PathUtils::toExecutableName("adb"))
                                     .c_str()));
    std::string output_file =
            dir->makeSubPath("Sdk/platform-tools/source.properties");
    std::ofstream ofs(output_file);
    ASSERT_TRUE(ofs.is_open());
    ofs << "### Comment\nArchive.HostOs=linux\nPkg.License=\\nNoliense\n"
           "Pkg.LicenseRef=android-sdk-license\nPkg.Revision=22.1.0\n"
           "Pkg.SourceUrl=https\\://dl.google.com/android/repository/"
           "repository-12.xml\n";
    ofs.close();
    std::string sdkRoot = PathUtils::join(dir->path(), "Sdk");
    system.envSet("ANDROID_SDK_ROOT", sdkRoot);
    auto adb = AdbInterface::create(nullptr);
    EXPECT_FALSE(adb->isAdbVersionCurrent());
    EXPECT_EQ("", adb->detectedAdbPath());
}
