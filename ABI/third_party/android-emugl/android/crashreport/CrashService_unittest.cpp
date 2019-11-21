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

#include "android/crashreport/CrashService.h"
#include "android/crashreport/testing/TestCrashSystem.h"

#include "android/base/testing/TestSystem.h"
#include "android/base/testing/TestTempDir.h"

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>
#include <inttypes.h>

using namespace android::base;
using namespace android::crashreport;

TEST(CrashService, get_set_dumpfile) {
    std::unique_ptr<CrashService> crash(
            CrashService::makeCrashService("foo", "bar", nullptr));
    std::string path("/test/foo/bar");
    crash->setDumpFile(path);
    EXPECT_STREQ(crash->getDumpFile().c_str(), path.c_str());
}

TEST(CrashService, validDumpFile) {
    TestSystem testsys("/launchdir", System::kProgramBitness, "/");
    TestTempDir* testDir = testsys.getTempRoot();
    testDir->makeSubDir("foo");
    testDir->makeSubFile("foo/bar.dmp");
    testDir->makeSubFile("foo/bar.dmp2");
    std::unique_ptr<CrashService> crash(
            CrashService::makeCrashService("foo", "bar", nullptr));
    std::string path;

    path = "foo/bar.dmp";
    crash->setDumpFile(path);
    EXPECT_TRUE(crash->validDumpFile());

    path = "foo/bar2.dmp";
    crash->setDumpFile(path);
    EXPECT_FALSE(crash->validDumpFile());

    path = "foo/bar.dmp2";
    crash->setDumpFile(path);
    EXPECT_FALSE(crash->validDumpFile());
}

// Unittest limited to linux as wine seems incompatible, and mac shows
// inconsistent results.
#ifdef __linux__

static std::string getTestCrasher() {
    std::string path = System::get()->getLauncherDirectory().c_str();
    path += System::kDirSeparator;
    path += "emulator";
    if (System::get()->getProgramBitness() == 64) {
        path += "64";
    }
    path += "_test_crasher";
#ifdef _WIN32
    path += ".exe";
#endif
    EXPECT_TRUE(System::get()->pathIsFile(path));
    return path;
}

static std::vector<std::string> getTestCrasherCmdLine(std::string pipe) {
    const std::vector<std::string> cmdline = {getTestCrasher(), "-pipe", pipe};
    return cmdline;
}

// invalidURLUpload hangs on wine, but passes on windows host
TEST(CrashService, invalidURLUpload) {
    TestTempDir crashdir("crashdir");
    TestCrashSystem crashsystem(crashdir.path(), "non-existent-domain-name-for-crash-reports.com");
    crashdir.makeSubFile("bar.dmp");
    std::unique_ptr<CrashService> crash(
            CrashService::makeCrashService("foo", "bar", nullptr));
    crash->setDumpFile(crashdir.makeSubPath("bar.dmp").c_str());
    EXPECT_FALSE(crash->uploadCrash());
}

// Crashing binary doesn't work well in Wine
// Fails intermittently on mac build systems
// Fails intermittantly on linux presubmit build
/*
TEST(CrashService, startAttachWaitCrash) {
    TestTempDir crashdir("crashdir");
    TestCrashSystem crashsystem(crashdir.path(), "localhost");
    std::unique_ptr<CrashService> crash(
            CrashService::makeCrashService("foo", "bar", nullptr));
    CrashSystem::CrashPipe crashpipe = CrashSystem::get()->getCrashPipe();
    EXPECT_TRUE(crashpipe.isValid());
    crash->startCrashServer(crashpipe.mServer);
    std::vector<std::string> cmdline = getTestCrasherCmdLine(crashpipe.mClient);

    int pid = CrashSystem::spawnService(cmdline);
    EXPECT_GT(pid, 0);
    int64_t waitduration = crash->waitForDumpFile(pid, 20000);
    EXPECT_NE(waitduration, -1);
    EXPECT_TRUE(CrashSystem::isDump(crash->getDumpFile()));
    crash->processCrash();
    std::string details = crash->getReport();
    EXPECT_NE(details.find("Crash reason:"), std::string::npos);
    EXPECT_NE(details.find("Thread 0"), std::string::npos);
    EXPECT_NE(details.find("Loaded modules:"), std::string::npos);
}
*/

TEST(CrashService, startAttachWaitNoCrash) {
    TestTempDir crashdir("crashdir");
    TestCrashSystem crashsystem(crashdir.path(), "localhost");
    std::unique_ptr<CrashService> crash(
            CrashService::makeCrashService("foo", "bar", nullptr));
    CrashSystem::CrashPipe crashpipe = CrashSystem::get()->getCrashPipe();
    EXPECT_TRUE(crashpipe.isValid());
    crash->startCrashServer(crashpipe.mServer);
    std::vector<std::string> cmdline = getTestCrasherCmdLine(crashpipe.mClient);
    cmdline.push_back(StringView("-nocrash"));
    int pid = CrashSystem::spawnService(cmdline);
    EXPECT_GT(pid, 0);
    int64_t waitduration = crash->waitForDumpFile(pid, 10000);
    EXPECT_EQ(waitduration, -1);
}

TEST(CrashService, startWaitNoAttach) {
    TestTempDir crashdir("crashdir");
    TestCrashSystem crashsystem(crashdir.path(), "localhost");
    std::unique_ptr<CrashService> crash(
            CrashService::makeCrashService("foo", "bar", nullptr));
    CrashSystem::CrashPipe crashpipe = CrashSystem::get()->getCrashPipe();
    EXPECT_TRUE(crashpipe.isValid());
    crash->startCrashServer(crashpipe.mServer);
    std::vector<std::string> cmdline = getTestCrasherCmdLine(crashpipe.mClient);
    cmdline.push_back(StringView("-noattach"));
    int pid = CrashSystem::spawnService(cmdline);
    EXPECT_GT(pid, 0);
    int64_t waitduration = crash->waitForDumpFile(pid, 1);
    EXPECT_EQ(waitduration, -1);
}

// Test fails regularly on one of the linux build machines
// failure happens when waitForDumpFile times out instead of
// finding a crash
//TEST(CrashService, startAttachWaitTimeout) {
//    TestTempDir crashdir("crashdir");
//    TestCrashSystem crashsystem(crashdir.path(), "localhost");
//    std::unique_ptr<CrashService> crash(
//            CrashService::makeCrashService("foo", "bar", nullptr));
//    CrashSystem::CrashPipe crashpipe = CrashSystem::get()->getCrashPipe();
//    EXPECT_TRUE(crashpipe.isValid());
//    crash->startCrashServer(crashpipe.mServer);
//    std::vector<std::string> cmdline = getTestCrasherCmdLine(crashpipe.mClient);
//    cmdline.push_back(StringView("-delay_ms"));
//    cmdline.push_back(StringView("100"));
//    int pid = CrashSystem::spawnService(cmdline);
//    EXPECT_GT(pid, 0);
//    int64_t waitduration = crash->waitForDumpFile(pid, 1);
//    EXPECT_EQ(waitduration, -1);
//    waitduration = crash->waitForDumpFile(pid, 200);
//    EXPECT_NE(waitduration, -1);
//}
#endif  // __linux__
