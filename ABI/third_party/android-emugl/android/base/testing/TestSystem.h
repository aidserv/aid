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

#pragma once

#include "android/base/Log.h"
#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"
#include "android/base/testing/TestTempDir.h"
#include "android/base/threads/Thread.h"
#include "android/utils/path.h"

namespace android {
namespace base {

// The TestSystem class provides a mock implementation that of the System that
// can be used by UnitTests. Instantiation will result in tempory replacement of
// the System singleton with this version, the prevous setting will be restored
// in the destructor of this object
//
// Some things to be aware of:
//
// Interaction with the filesystem will result in the creation of
// a temporary directory that will be used to interact with the
// filesystem. This temporary directory is deleted in the destructor.
//
// Path resolution is done as follows:
//   - Relative paths are resolved starting from current directory
//     which by default is: "/home"
//   - The construction of this object does not create the launcherDir,
//     appDataDir or home dir. If you need these directories to exist you will
//     have to create them as follows: getTempRoot()->makeSubDir("home").
//   - Path resolution can result in switching / into \ when running under
//      Win32. If you are doing anything with paths
//     it is best to include "android/utils/path.h" and use the PATH_SEP
//     and PATH_SEP_C macros to indicate path separators in your strings.
//
class TestSystem : public System {
public:
    TestSystem(StringView launcherDir,
               int hostBitness,
               StringView homeDir = "/home",
               StringView appDataDir = "")
        : mProgramDir(launcherDir),
          mProgramSubdir(""),
          mLauncherDir(launcherDir),
          mHomeDir(homeDir),
          mAppDataDir(appDataDir),
          mCurrentDir(homeDir),
          mHostBitness(hostBitness),
          mIsRemoteSession(false),
          mRemoteSessionType(),
          mTempDir(NULL),
          mTempRootPrefix(),
          mEnvPairs(),
          mPrevSystem(System::setForTesting(this)),
          mTimes(),
          mShellFunc(NULL),
          mShellOpaque(NULL),
          mUnixTime(),
          mPid() {
          }

    virtual ~TestSystem() {
        System::setForTesting(mPrevSystem);
        delete mTempDir;
    }

    virtual const std::string& getProgramDirectory() const override {
        return mProgramDir;
    }

    // Set directory of currently executing binary.  This must be a subdirectory
    // of mLauncherDir and specified relative to mLauncherDir
    void setProgramSubDir(StringView programSubDir) {
        mProgramSubdir = programSubDir;
        if (programSubDir.empty()) {
            mProgramDir = getLauncherDirectory();
        } else {
            mProgramDir =
                    PathUtils::join(getLauncherDirectory(), programSubDir);
        }
    }

    virtual const std::string& getLauncherDirectory() const override {
        if (mLauncherDir.size()) {
            return mLauncherDir;
        } else {
            return getTempRoot()->pathString();
        }
    }

    void setLauncherDirectory(StringView launcherDir) {
        mLauncherDir = launcherDir;
        // Update directories that are suffixes of |mLauncherDir|.
        setProgramSubDir(mProgramSubdir);
    }

    virtual const std::string& getHomeDirectory() const override { return mHomeDir; }

    void setHomeDirectory(StringView homeDir) { mHomeDir = homeDir; }

    virtual const std::string& getAppDataDirectory() const override {
        return mAppDataDir;
    }

    void setAppDataDirectory(StringView appDataDir) {
        mAppDataDir = appDataDir;
    }

    virtual std::string getCurrentDirectory() const override { return mCurrentDir; }

    // Set current directory during unit-testing.
    void setCurrentDirectoryForTesting(StringView path) { mCurrentDir = path; }

    virtual int getHostBitness() const override { return mHostBitness; }

    virtual OsType getOsType() const override { return mOsType; }

    virtual std::string getOsName() override { return mOsName; }

    virtual bool isRunningUnderWine() const override { return mUnderWine; }

    void setRunningUnderWine(bool underWine) { mUnderWine = underWine; }

    virtual Pid getCurrentProcessId() const override { return mPid; }

    virtual MemUsage getMemUsage() override {
        MemUsage res;
        res.resident = 4294967295ULL;
        res.resident_max = 4294967295ULL * 2;
        res.virt = 4294967295ULL * 4;
        res.virt_max = 4294967295ULL * 8;
        res.total_phys_memory = 4294967295ULL * 16;
        res.total_page_file = 4294967295ULL * 32;
        return res;
    }

    void setCurrentProcessId(Pid pid) { mPid = pid; }

    void setOsType(OsType type) { mOsType = type; }

    virtual std::string envGet(StringView varname) const override {
        for (size_t n = 0; n < mEnvPairs.size(); n += 2) {
            const std::string& name = mEnvPairs[n];
            if (name == varname) {
                return mEnvPairs[n + 1];
            }
        }
        return std::string();
    }

    virtual std::vector<std::string> envGetAll() const override {
        std::vector<std::string> res;
        for (size_t i = 0; i < mEnvPairs.size(); i += 2) {
            const std::string& name = mEnvPairs[i];
            const std::string& val = mEnvPairs[i + 1];
            res.push_back(name + "=" + val);
        }
        return res;
    }

    virtual void envSet(StringView varname, StringView varvalue) override {
        // First, find if the name is in the array.
        int index = -1;
        for (size_t n = 0; n < mEnvPairs.size(); n += 2) {
            if (mEnvPairs[n] == varname) {
                index = static_cast<int>(n);
                break;
            }
        }
        if (varvalue.empty()) {
            // Remove definition, if any.
            if (index >= 0) {
                mEnvPairs.erase(mEnvPairs.begin() + index,
                                mEnvPairs.begin() + index + 2);
            }
        } else {
            if (index >= 0) {
                // Replacement.
                mEnvPairs[index + 1] = varvalue;
            } else {
                // Addition.
                mEnvPairs.push_back(varname);
                mEnvPairs.push_back(varvalue);
            }
        }
    }

    virtual bool envTest(StringView varname) const override {
        for (size_t n = 0; n < mEnvPairs.size(); n += 2) {
            const std::string& name = mEnvPairs[n];
            if (name == varname) {
                return true;
            }
        }
        return false;
    }

    virtual bool pathExists(StringView path) const override {
        return pathExistsInternal(toTempRoot(path));
    }

    virtual bool pathIsFile(StringView path) const override {
        return pathIsFileInternal(toTempRoot(path));
    }

    virtual bool pathIsDir(StringView path) const override {
        return pathIsDirInternal(toTempRoot(path));
    }

    virtual bool pathCanRead(StringView path) const override {
        return pathCanReadInternal(toTempRoot(path));
    }

    virtual bool pathCanWrite(StringView path) const override {
        return pathCanWriteInternal(toTempRoot(path));
    }

    virtual bool pathCanExec(StringView path) const override {
        return pathCanExecInternal(toTempRoot(path));
    }

    virtual bool deleteFile(StringView path) const override {
        return deleteFileInternal(toTempRoot(path));
    }

    virtual bool pathFileSize(StringView path,
                              FileSize* outFileSize) const override {
        return pathFileSizeInternal(toTempRoot(path), outFileSize);
    }

    virtual Optional<Duration> pathCreationTime(
            StringView path) const override {
        return pathCreationTimeInternal(toTempRoot(path));
    }

    virtual std::vector<std::string> scanDirEntries(
            StringView dirPath,
            bool fullPath = false) const override {
        getTempRoot();  // make sure we have a temp root;

        std::string newPath = toTempRoot(dirPath);
        std::vector<std::string> result = scanDirInternal(newPath);
        if (fullPath) {
            std::string prefix = PathUtils::addTrailingDirSeparator(dirPath);
            for (size_t n = 0; n < result.size(); ++n) {
                result[n] = prefix + result[n];
            }
        }
        return result;
    }

    virtual TestTempDir* getTempRoot() const {
        if (!mTempDir) {
            mTempDir = new TestTempDir("TestSystem");
            mTempRootPrefix = PathUtils::addTrailingDirSeparator(
                    std::string(mTempDir->path()));
        }
        return mTempDir;
    }

    virtual bool isRemoteSession(std::string* sessionType) const override {
        if (!mIsRemoteSession) {
            return false;
        }
        *sessionType = mRemoteSessionType;
        return true;
    }

    // Force the remote session type. If |sessionType| is NULL or empty,
    // this sets the session as local. Otherwise, |*sessionType| must be
    // a session type.
    void setRemoteSessionType(StringView sessionType) {
        mIsRemoteSession = !sessionType.empty();
        if (mIsRemoteSession) {
            mRemoteSessionType = sessionType;
        }
    }

    virtual Times getProcessTimes() const override { return mTimes; }

    void setProcessTimes(const Times& times) { mTimes = times; }

    // Type of a helper function that can be used during unit-testing to
    // receive the parameters of a runCommand() call. Register it
    // with setShellCommand().
    typedef bool(ShellCommand)(void* opaque,
                               const std::vector<std::string>& commandLine,
                               System::Duration timeoutMs,
                               System::ProcessExitCode* outExitCode,
                               System::Pid* outChildPid,
                               const std::string& outputFile);

    // Register a silent shell function. |shell| is the function callback,
    // and |shellOpaque| a user-provided pointer passed as its first parameter.
    void setShellCommand(ShellCommand* shell, void* shellOpaque) {
        mShellFunc = shell;
        mShellOpaque = shellOpaque;
    }

    bool runCommand(const std::vector<std::string>& commandLine,
                    RunOptions options,
                    System::Duration timeoutMs,
                    System::ProcessExitCode* outExitCode,
                    System::Pid* outChildPid,
                    const std::string& outputFile) override {
        if (!commandLine.size()) {
            return false;
        }
        // If a silent shell function was registered, invoke it, otherwise
        // ignore the command completely.
        bool result = true;

        if (mShellFunc) {
            result = (*mShellFunc)(mShellOpaque, commandLine, timeoutMs,
                                   outExitCode, outChildPid, outputFile);
        }

        return result;
    }

    virtual std::string getTempDir() const override { return "/tmp"; }

    virtual time_t getUnixTime() const override {
        return getUnixTimeUs() / 1000000;
    }

    virtual Duration getUnixTimeUs() const override {
        return getHighResTimeUs();
    }

    virtual WallDuration getHighResTimeUs() const override {
        if (mUnixTimeLive) {
            auto now = hostSystem()->getHighResTimeUs();
            mUnixTime += now - mUnixTimeLastQueried;
            mUnixTimeLastQueried = now;
        }
        return mUnixTime;
    }

    void setUnixTime(time_t time) { setUnixTimeUs(time * 1000000LL); }

    void setUnixTimeUs(Duration time) {
        mUnixTimeLastQueried = mUnixTime = time;
    }

    void setLiveUnixTime(bool enable) {
        mUnixTimeLive = enable;
        if (enable) {
            mUnixTimeLastQueried = hostSystem()->getHighResTimeUs();
        }
    }

    virtual void sleepMs(unsigned n) const override {
        // Don't sleep in tests, use the static functions from Thread class
        // if you need a delay (you don't!).
        Thread::yield();  // Add a small delay to mimic the intended behavior.
    }

    virtual void yield() const override { Thread::yield(); }

private:
    std::string toTempRoot(StringView pathView) const {
        std::string path = pathView;
        if (!PathUtils::isAbsolute(path)) {
            auto currdir = getCurrentDirectory();
            path = currdir + PATH_SEP + path;
        }

        // mTempRootPrefix ends with a dir separator, ignore it for comparison.
        StringView prefix(mTempRootPrefix.c_str(), mTempRootPrefix.size() - 1);
        if (prefix.size() <= path.size() &&
            prefix == StringView(path.c_str(), prefix.size()) &&
            (prefix.size() == path.size() ||
             PathUtils::isDirSeparator(path[prefix.size()]))) {
            // Avoid prepending prefix if it's already there.
            return path;
        } else {
            // Resolve ., .. and replacing \ or / with PATH_SEP
            auto parts = PathUtils::decompose(mTempRootPrefix + path);
            PathUtils::simplifyComponents(&parts);
            auto res = PathUtils::recompose(parts);
            return res;
        }
    }

    std::string mProgramDir;
    std::string mProgramSubdir;
    std::string mLauncherDir;
    std::string mHomeDir;
    std::string mAppDataDir;
    std::string mCurrentDir;
    int mHostBitness;
    bool mIsRemoteSession;
    std::string mRemoteSessionType;
    mutable TestTempDir* mTempDir;
    mutable std::string mTempRootPrefix;
    std::vector<std::string> mEnvPairs;
    System* mPrevSystem;
    Times mTimes;
    ShellCommand* mShellFunc;
    void* mShellOpaque;
    mutable Duration mUnixTime;
    mutable Duration mUnixTimeLastQueried = 0;
    bool mUnixTimeLive = false;
    OsType mOsType = OsType::Windows;
    std::string mOsName = "";
    bool mUnderWine = false;
    Pid mPid = 0;
};

}  // namespace base
}  // namespace android
