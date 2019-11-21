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

#include "android/qt/qt_setup.h"

#include "android/base/testing/TestSystem.h"
#include "android/base/testing/TestTempDir.h"

#include <gtest/gtest.h>

using namespace android::base;

TEST(androidQtSetupEnv, NoQtLibDir) {
    TestSystem testSys32("/foo", 32);
    TestTempDir* testDir32 = testSys32.getTempRoot();
    EXPECT_TRUE(testDir32->makeSubDir("foo"));
    EXPECT_TRUE(testDir32->makeSubDir("foo/lib"));
    EXPECT_TRUE(testDir32->makeSubDir("foo/lib64"));
    EXPECT_FALSE(androidQtSetupEnv(32));

    TestSystem testSys64("/foo", 64);
    TestTempDir* testDir64 = testSys64.getTempRoot();
    EXPECT_TRUE(testDir64->makeSubDir("foo"));
    EXPECT_TRUE(testDir64->makeSubDir("foo/lib"));
    EXPECT_TRUE(testDir64->makeSubDir("foo/lib64"));
    EXPECT_FALSE(androidQtSetupEnv(64));
}

TEST(androidQtSetupEnv, QtLibDir32Only) {
    TestSystem testSys32("/foo", 32);
    TestTempDir* testDir32 = testSys32.getTempRoot();
    EXPECT_TRUE(testDir32->makeSubDir("foo"));
    EXPECT_TRUE(testDir32->makeSubDir("foo/lib"));
    EXPECT_TRUE(testDir32->makeSubDir("foo/lib/qt"));
    EXPECT_TRUE(testDir32->makeSubDir("foo/lib/qt/lib"));
    EXPECT_TRUE(testDir32->makeSubDir("foo/lib64"));
    EXPECT_TRUE(androidQtSetupEnv(32));

    TestSystem testSys64("/foo", 64);
    TestTempDir* testDir64 = testSys64.getTempRoot();
    EXPECT_TRUE(testDir64->makeSubDir("foo"));
    EXPECT_TRUE(testDir64->makeSubDir("foo/lib"));
    EXPECT_TRUE(testDir64->makeSubDir("foo/lib/qt"));
    EXPECT_TRUE(testDir64->makeSubDir("foo/lib/qt/lib"));
    EXPECT_TRUE(testDir64->makeSubDir("foo/lib64"));
    EXPECT_FALSE(androidQtSetupEnv(64));
}

TEST(androidQtSetupEnv, QtLibDir64Only) {
    TestSystem testSys32("/foo", 32);
    TestTempDir* testDir32 = testSys32.getTempRoot();
    EXPECT_TRUE(testDir32->makeSubDir("foo"));
    EXPECT_TRUE(testDir32->makeSubDir("foo/lib"));
    EXPECT_TRUE(testDir32->makeSubDir("foo/lib64"));
    EXPECT_TRUE(testDir32->makeSubDir("foo/lib64/qt"));
    EXPECT_TRUE(testDir32->makeSubDir("foo/lib64/qt/lib"));
    EXPECT_FALSE(androidQtSetupEnv(32));

    TestSystem testSys64("/foo", 64);
    TestTempDir* testDir64 = testSys64.getTempRoot();
    EXPECT_TRUE(testDir64->makeSubDir("foo"));
    EXPECT_TRUE(testDir64->makeSubDir("foo/lib"));
    EXPECT_TRUE(testDir64->makeSubDir("foo/lib64"));
    EXPECT_TRUE(testDir64->makeSubDir("foo/lib64/qt"));
    EXPECT_TRUE(testDir64->makeSubDir("foo/lib64/qt/lib"));
    EXPECT_TRUE(androidQtSetupEnv(64));
}

TEST(androidQtSetupEnv, QtLibDir32And64) {
    TestSystem testSys32("/foo", 32);
    TestTempDir* testDir32 = testSys32.getTempRoot();
    EXPECT_TRUE(testDir32->makeSubDir("foo"));
    EXPECT_TRUE(testDir32->makeSubDir("foo/lib"));
    EXPECT_TRUE(testDir32->makeSubDir("foo/lib/qt"));
    EXPECT_TRUE(testDir32->makeSubDir("foo/lib/qt/lib"));
    EXPECT_TRUE(testDir32->makeSubDir("foo/lib64"));
    EXPECT_TRUE(testDir32->makeSubDir("foo/lib64/qt"));
    EXPECT_TRUE(testDir32->makeSubDir("foo/lib64/qt/lib"));
    EXPECT_TRUE(androidQtSetupEnv(32));

    TestSystem testSys64("/foo", 64);
    TestTempDir* testDir64 = testSys64.getTempRoot();
    EXPECT_TRUE(testDir64->makeSubDir("foo"));
    EXPECT_TRUE(testDir64->makeSubDir("foo/lib"));
    EXPECT_TRUE(testDir64->makeSubDir("foo/lib/qt"));
    EXPECT_TRUE(testDir64->makeSubDir("foo/lib/qt/lib"));
    EXPECT_TRUE(testDir64->makeSubDir("foo/lib64"));
    EXPECT_TRUE(testDir64->makeSubDir("foo/lib64/qt"));
    EXPECT_TRUE(testDir64->makeSubDir("foo/lib64/qt/lib"));
    EXPECT_TRUE(androidQtSetupEnv(64));
}

TEST(androidQtSetupEnv, DetectBitness) {
    TestSystem testSys("/foo", System::kProgramBitness);
    TestTempDir* testDir = testSys.getTempRoot();
    EXPECT_TRUE(testDir->makeSubDir("foo"));
    EXPECT_TRUE(testDir->makeSubDir("foo/lib"));
    EXPECT_TRUE(testDir->makeSubDir("foo/lib/qt"));
    EXPECT_TRUE(testDir->makeSubDir("foo/lib/qt/lib"));
    EXPECT_TRUE(testDir->makeSubDir("foo/lib64"));
    EXPECT_TRUE(testDir->makeSubDir("foo/lib64/qt"));
    EXPECT_TRUE(testDir->makeSubDir("foo/lib64/qt/lib"));
    EXPECT_TRUE(androidQtSetupEnv(0));
}
