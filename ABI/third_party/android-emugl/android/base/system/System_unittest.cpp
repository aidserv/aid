// Copyright (C) 2015 The Android Open Source Project
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

#include "android/base/system/System.h"

#include "android/base/EintrWrapper.h"
#include "android/base/files/PathUtils.h"
#include "android/base/Log.h"
#include "android/base/testing/TestSystem.h"
#include "android/base/testing/TestTempDir.h"

#include <gtest/gtest.h>

#include <fstream>
#include <iostream>
#include <string>

#include <fcntl.h>
#include <unistd.h>

#define ARRAYLEN(x)  (sizeof(x)/sizeof(x[0]))

namespace android {
namespace base {

static void make_subfile(const std::string& dir, const char* file) {
    std::string path = dir;
    path.append("/");
    path.append(file);
    int fd = ::open(path.c_str(), O_WRONLY|O_CREAT, 0755);
    EXPECT_GE(fd, 0) << "Path: " << path.c_str();
    ::close(fd);
}

TEST(System, get) {
    System* sys1 = System::get();
    EXPECT_TRUE(sys1);

    System* sys2 = System::get();
    EXPECT_EQ(sys1, sys2);
}

TEST(System, getProgramDirectory) {
    std::string dir = System::get()->getProgramDirectory();
    EXPECT_FALSE(dir.empty());
    LOG(INFO) << "Program directory: [" << dir.c_str() << "]";
}

TEST(System, getHomeDirectory) {
    std::string dir = System::get()->getHomeDirectory();
    EXPECT_FALSE(dir.empty());
    LOG(INFO) << "Home directory: [" << dir.c_str() << "]";
}

TEST(System, getAppDataDirectory) {
    std::string dir = System::get()->getAppDataDirectory();
#if defined(__linux__)
    EXPECT_TRUE(dir.empty());
#else
    // Mac OS X, Microsoft Windows
    EXPECT_FALSE(dir.empty());
#endif // __linux__
    LOG(INFO) << "AppData directory: [" << dir.c_str() << "]";
}

TEST(System, getCurrentDirectory) {
    std::string dir = System::get()->getCurrentDirectory();
    EXPECT_FALSE(dir.empty());
    LOG(INFO) << "Current directory: [" << dir.c_str() << "]";
}

// Tests case where program directory == launcher directory (QEMU1)
TEST(TestSystem, getDirectory) {
    const char kLauncherDir[] = "/foo/bar";
    const char  kHomeDir[]    = "/mama/papa";
#if defined(__linux__)
    const char *kAppDataDir   = "";
#else
    // Mac OS X, Microsoft Windows
    const char  kAppDataDir[] = "/lala/kaka";
#endif // __linux__
    TestSystem testSys(kLauncherDir, 32, kHomeDir, kAppDataDir);
    std::string ldir = System::get()->getLauncherDirectory();
    EXPECT_STREQ(kLauncherDir, ldir.c_str());
    std::string pdir = System::get()->getProgramDirectory();
    EXPECT_STREQ(kLauncherDir, pdir.c_str());
    std::string hdir = System::get()->getHomeDirectory();
    EXPECT_STREQ(kHomeDir, hdir.c_str());
    std::string adir = System::get()->getAppDataDirectory();
#if defined(__linux__)
    EXPECT_TRUE(adir.empty());
#else
    // Mac OS X, Microsoft Windows
    EXPECT_STREQ(kAppDataDir, adir.c_str());
#endif  // __linux__
}

// Tests case where program directory is a subdirectory of launcher directory
// (QEMU2)
TEST(TestSystem, getDirectoryProgramDir) {
    const char kLauncherDir[] = "/foo/bar";
    const char kProgramDir[] = "qemu/os-arch";
    TestSystem testSys(kLauncherDir, 32, "/home", "/app");
    testSys.setProgramSubDir(kProgramDir);

    std::string ldir = System::get()->getLauncherDirectory();
    EXPECT_STREQ(kLauncherDir, ldir.c_str());
    std::string pdir = System::get()->getProgramDirectory();
#ifdef _WIN32
    EXPECT_STREQ("/foo/bar\\qemu/os-arch", pdir.c_str());
#else
    EXPECT_STREQ("/foo/bar/qemu/os-arch", pdir.c_str());
#endif
}

TEST(System, getHostBitness) {
    int hostBitness = System::get()->getHostBitness();
    LOG(INFO) << "Host bitness: " << hostBitness;
    EXPECT_TRUE(hostBitness == 32 || hostBitness == 64);

    {
        TestSystem sysTest("/foo", 32);
        EXPECT_EQ(32, System::get()->getHostBitness());
    }
    {
        TestSystem sysTest("/foo64", 64);
        EXPECT_EQ(64, System::get()->getHostBitness());
    }
}

TEST(System, getProgramBitness) {
    const int kExpected = (sizeof(void*) == 8) ? 64 : 32;
    EXPECT_EQ(kExpected, System::get()->getProgramBitness());
}

TEST(System, getOsName) {
    std::string osName = System::get()->getOsName();
    LOG(INFO) << "Host OS: " << osName;
    EXPECT_STRNE("Error: ", osName.substr(0, 7).c_str());
}

TEST(System, scandDirEntries) {
    static const char* const kExpected[] = {
        "fifth", "first", "fourth", "second", "sixth", "third"
    };
    static const char* const kInput[] = {
        "first", "second", "third", "fourth", "fifth", "sixth"
    };
    const size_t kCount = ARRAYLEN(kInput);

    TestTempDir myDir("scanDirEntries");
    for (size_t n = 0; n < kCount; ++n) {
        make_subfile(myDir.path(), kInput[n]);
    }

    std::vector<std::string> entries =
            System::get()->scanDirEntries(myDir.path());

    EXPECT_EQ(kCount, entries.size());
    for (size_t n = 0; n < kCount; ++n) {
        EXPECT_STREQ(kExpected[n], entries[n].c_str()) << "#" << n;
    }
}

TEST(System, envGetAndSet) {
    System* sys = System::get();
    const char kVarName[] = "FOO_BAR_TESTING_STUFF";
    const char kVarValue[] = "SomethingCompletelyRandomForYou!";

    EXPECT_FALSE(sys->envTest(kVarName));
    EXPECT_STREQ("", sys->envGet(kVarName).c_str());
    sys->envSet(kVarName, kVarValue);
    EXPECT_TRUE(sys->envTest(kVarName));
    EXPECT_STREQ(kVarValue, sys->envGet(kVarName).c_str());
    sys->envSet(kVarName, nullptr);
    EXPECT_FALSE(sys->envTest(kVarName));
    EXPECT_STREQ("", sys->envGet(kVarName).c_str());
}

TEST(System, pathIsDir) {
    TestSystem sys("/bin", 32, "/");

    EXPECT_FALSE(sys.pathIsDir("foo"));
    EXPECT_FALSE(sys.pathIsDir("foo/"));
#ifdef _WIN32
    EXPECT_FALSE(sys.pathIsDir("foo\\"));
#endif

    sys.getTempRoot()->makeSubDir("foo");

    EXPECT_TRUE(sys.pathIsDir("foo"));
    EXPECT_TRUE(sys.pathIsDir("foo/"));
#ifdef _WIN32
    EXPECT_TRUE(sys.pathIsDir("foo\\"));
#endif
}

TEST(System, pathOperations) {
    System* sys = System::get();
    TestTempDir tempDir("path_opts");
    std::string fooPath = tempDir.path();
    fooPath += "/foo";
    System::FileSize fileSize;

    EXPECT_FALSE(sys->pathExists(fooPath));
    EXPECT_FALSE(sys->pathIsFile(fooPath));
    EXPECT_FALSE(sys->pathIsDir(fooPath));
    EXPECT_FALSE(sys->pathCanRead(fooPath));
    EXPECT_FALSE(sys->pathCanWrite(fooPath));
    EXPECT_FALSE(sys->pathCanExec(fooPath));
    EXPECT_FALSE(sys->pathFileSize(fooPath, &fileSize));

    const auto createTimeBefore = sys->getUnixTimeUs();
    make_subfile(tempDir.path(), "foo");
    const auto createTimeAfter = sys->getUnixTimeUs();

    EXPECT_TRUE(sys->pathExists(fooPath));
    EXPECT_TRUE(sys->pathIsFile(fooPath));
    EXPECT_FALSE(sys->pathIsDir(fooPath));

    // NOTE: Windows doesn't have 'execute' permission bits.
    // Any readable file can be executed. Also any writable file
    // is readable.
    EXPECT_FALSE(HANDLE_EINTR(chmod(fooPath.c_str(), S_IRUSR|S_IWUSR|S_IXUSR)));
    EXPECT_TRUE(sys->pathCanRead(fooPath));
    EXPECT_TRUE(sys->pathCanWrite(fooPath));
    EXPECT_TRUE(sys->pathCanExec(fooPath));

    EXPECT_FALSE(HANDLE_EINTR(chmod(fooPath.c_str(), S_IRUSR)));
    EXPECT_TRUE(sys->pathCanRead(fooPath));
    EXPECT_FALSE(sys->pathCanWrite(fooPath));
#ifdef _WIN32
    EXPECT_TRUE(sys->pathCanExec(fooPath));
#else
    EXPECT_FALSE(sys->pathCanExec(fooPath));
#endif

    EXPECT_FALSE(HANDLE_EINTR(chmod(fooPath.c_str(), S_IWUSR)));
#ifdef _WIN32
    EXPECT_TRUE(sys->pathCanRead(fooPath));
    EXPECT_TRUE(sys->pathCanWrite(fooPath));
    EXPECT_TRUE(sys->pathCanExec(fooPath));
#else
    EXPECT_FALSE(sys->pathCanRead(fooPath));
    EXPECT_TRUE(sys->pathCanWrite(fooPath));
    EXPECT_FALSE(sys->pathCanExec(fooPath));
#endif

    EXPECT_FALSE(HANDLE_EINTR(chmod(fooPath.c_str(), S_IXUSR)));
#ifdef _WIN32
    EXPECT_TRUE(sys->pathCanRead(fooPath));
#else
    EXPECT_FALSE(sys->pathCanRead(fooPath));
#endif
    EXPECT_FALSE(sys->pathCanWrite(fooPath));
    EXPECT_TRUE(sys->pathCanExec(fooPath));

    EXPECT_FALSE(HANDLE_EINTR(chmod(fooPath.c_str(), S_IRUSR|S_IWUSR)));
    EXPECT_TRUE(sys->pathCanRead(fooPath));
    EXPECT_TRUE(sys->pathCanWrite(fooPath));
#ifdef _WIN32
    EXPECT_TRUE(sys->pathCanExec(fooPath));
#else
    EXPECT_FALSE(sys->pathCanExec(fooPath));
#endif

    EXPECT_FALSE(HANDLE_EINTR(chmod(fooPath.c_str(), S_IRUSR|S_IXUSR)));
    EXPECT_TRUE(sys->pathCanRead(fooPath));
    EXPECT_FALSE(sys->pathCanWrite(fooPath));
    EXPECT_TRUE(sys->pathCanExec(fooPath));

    EXPECT_FALSE(HANDLE_EINTR(chmod(fooPath.c_str(), S_IWUSR|S_IXUSR)));
#ifdef _WIN32
    EXPECT_TRUE(sys->pathCanRead(fooPath));
#else
    EXPECT_FALSE(sys->pathCanRead(fooPath));
#endif
    EXPECT_TRUE(sys->pathCanWrite(fooPath));
    EXPECT_TRUE(sys->pathCanExec(fooPath));

    EXPECT_FALSE(sys->pathFileSize(fooPath, nullptr));
    EXPECT_TRUE(sys->pathFileSize(fooPath, &fileSize));
    EXPECT_EQ(0U, fileSize);

    std::ofstream fooFile(fooPath);
    ASSERT_TRUE(bool(fooFile));
    fooFile << "Some non-zero data";
    fooFile.close();
    EXPECT_TRUE(sys->pathFileSize(fooPath, &fileSize));
    EXPECT_LT(0U, fileSize);

    // Test creation time getter.
    auto createTime = sys->pathCreationTime(fooPath);
#ifdef __linux__
    EXPECT_FALSE(createTime);
    // Just to make the variables used.
    EXPECT_TRUE(createTimeBefore <= createTimeAfter);
#else
    ASSERT_TRUE(createTime);
    // On Windows creation time only contains seconds, so we need to make sure
    // we're comparing with the right precision.
    EXPECT_GE(*createTime, createTimeBefore - (createTimeBefore % 1000000));
    EXPECT_LE(*createTime, createTimeAfter);
#endif
    // Test file deletion
    EXPECT_TRUE(sys->deleteFile(fooPath));
    EXPECT_FALSE(sys->pathFileSize(fooPath, &fileSize));
}

TEST(System, scanDirEntriesWithFullPaths) {
    static const char* const kExpected[] = {
        "fifth", "first", "fourth", "second", "sixth", "third"
    };
    static const char* const kInput[] = {
        "first", "second", "third", "fourth", "fifth", "sixth"
    };
    const size_t kCount = ARRAYLEN(kInput);

    TestTempDir myDir("scanDirEntriesFull");
    for (size_t n = 0; n < kCount; ++n) {
        make_subfile(myDir.path(), kInput[n]);
    }

    std::vector<std::string> entries =
            System::get()->scanDirEntries(myDir.path(), true);

    EXPECT_EQ(kCount, entries.size());
    for (size_t n = 0; n < kCount; ++n) {
        std::string expected(myDir.path());
        expected = PathUtils::addTrailingDirSeparator(expected);
        expected += kExpected[n];
        EXPECT_STREQ(expected.c_str(), entries[n].c_str()) << "#" << n;
    }
}

TEST(System, isRemoteSession) {
    std::string sessionType;
    bool isRemote = System::get()->isRemoteSession(&sessionType);
    if (isRemote) {
        LOG(INFO) << "Remote session type [" << sessionType.c_str() << "]";
    } else {
        LOG(INFO) << "Local session type";
    }
}

TEST(System, addLibrarySearchDir) {
    TestSystem testSys("/foo/bar", 32);
    TestTempDir* testDir = testSys.getTempRoot();
    ASSERT_TRUE(testDir->makeSubDir("lib"));
    testSys.addLibrarySearchDir("lib");
}

TEST(System, findBundledExecutable) {
#ifdef _WIN32
    static const char kProgramFile[] = "myprogram.exe";
#else
    static const char kProgramFile[] = "myprogram";
#endif

    TestSystem testSys("/foo", System::kProgramBitness);
    TestTempDir* testDir = testSys.getTempRoot();
    ASSERT_TRUE(testDir->makeSubDir("foo"));

    std::vector<std::string> pathList;
    pathList.push_back(std::string("foo"));
    pathList.push_back(std::string(System::kBinSubDir));
    ASSERT_TRUE(testDir->makeSubDir(PathUtils::recompose(pathList).c_str()));

    pathList.push_back(std::string(kProgramFile));
    std::string programPath = PathUtils::recompose(pathList);
    make_subfile(testDir->path(), programPath.c_str());

    std::string path = testSys.findBundledExecutable("myprogram");
    std::string expectedPath("/");
    expectedPath += programPath;
    EXPECT_STREQ(expectedPath.c_str(), path.c_str());

    path = testSys.findBundledExecutable("otherprogram");
    EXPECT_FALSE(path.size());
}

TEST(System, getProcessTimes) {
    const System::Times times1 = System::get()->getProcessTimes();
    const System::Times times2 = System::get()->getProcessTimes();
    ASSERT_GE(times2.userMs, times1.userMs);
    ASSERT_GE(times2.systemMs, times1.systemMs);
}

TEST(System, getUnixTime) {
    const time_t curTime = time(NULL);
    const time_t time1 = System::get()->getUnixTime();
    const time_t time2 = System::get()->getUnixTime();
    ASSERT_GE(time1, curTime);
    ASSERT_GE(time2, time1);
}

TEST(System, runCommandTrue) {
#ifndef _WIN32
    std::vector<std::string> cmd = {"ls"};
#else
    // 'ver' is a builtin command, so all you need is a sane cmd.exe
    std::vector<std::string> cmd = {"cmd.exe", "/C", "ver"};
#endif

    EXPECT_TRUE(System::get()->runCommand(cmd));
    EXPECT_TRUE(System::get()->runCommand(cmd, RunOptions::WaitForCompletion));

    System::Pid pid = 0;
    System::ProcessExitCode exitCode = 666;
    EXPECT_TRUE(System::get()->runCommand(cmd, RunOptions::WaitForCompletion,
                                          System::kInfinite, &exitCode, &pid));
    EXPECT_EQ(0U, exitCode);
    EXPECT_GT(pid, 0U);
}

TEST(System, runCommandTimeout) {
#ifndef _WIN32
    std::vector<std::string> cmd = {"sleep", "0.1"};
#else
    // 2 Attempts give us a delay of 1 second.
    // 'ping' is not an internal cmd.exe command, but seems always being shipped
    // on recent windows boxes and wine.
    std::vector<std::string> cmd = {"ping", "-n", "2", "127.0.0.1"};
#endif

    EXPECT_FALSE(
            System::get()->runCommand(cmd, RunOptions::WaitForCompletion, 1));

    System::Pid pid = 0;
    System::ProcessExitCode exitCode = 666;
#ifdef _WIN32
    // Make sure we don't wait for too long here, where we expect to succeed.
    cmd = {"cmd.exe", "/C", "ver"};
#endif
    EXPECT_TRUE(System::get()->runCommand(cmd, RunOptions::WaitForCompletion,
                                          System::kInfinite, &exitCode, &pid));
    EXPECT_EQ(0U, exitCode);
    EXPECT_GT(pid, 0U);
}

TEST(System, DISABLED_runCommandWithOutput) {
    std::vector<std::string> cmd = {"echo", "hello"};
    System::Pid pid = 666;
    System::ProcessExitCode exitCode = 0;
    std::string outputFile = PathUtils::recompose(
            std::vector<StringView>{System::get()->getTempDir(), "test.txt"});

    EXPECT_TRUE(System::get()->runCommand(
            cmd, RunOptions::WaitForCompletion | RunOptions::DumpOutputToFile,
            System::kInfinite, &exitCode, &pid, outputFile));

    EXPECT_TRUE(System::get()->pathExists(outputFile));
    EXPECT_TRUE(System::get()->pathIsFile(outputFile));
    EXPECT_TRUE(System::get()->pathCanRead(outputFile));

    std::ifstream file(outputFile);
    EXPECT_TRUE(file.is_open());

    if (file.is_open()) {
        std::string line;
        EXPECT_TRUE(static_cast<bool>(std::getline(file, line)));

        // An idosyncrasy of cmd.exe's "echo" command - we add spaces between
        // arguments, and that space gets captured by echo. This is only a
        // problem with echo itself.
        EXPECT_TRUE(!strncmp("hello", line.c_str(), 5));

        file.close();
    }

    EXPECT_EQ(0, std::remove(outputFile.c_str()));
}

}  // namespace base
}  // namespace android
