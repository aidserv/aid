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

#include "android/crashreport/CrashSystem.h"

#include "android/base/files/PathUtils.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/StringFormat.h"
#include "android/base/system/System.h"
#ifdef _WIN32
#include "android/base/system/Win32UnicodeString.h"
#include "android/base/system/Win32Utils.h"
#endif
#include "android/crashreport/CrashReporter.h"
#include "android/emulation/ConfigDirs.h"
#include "android/utils/debug.h"
#include "android/utils/path.h"

#if defined(__linux__)
#include "client/linux/crash_generation/crash_generation_server.h"
#endif

#ifdef _WIN32
#include <process.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

#ifdef __APPLE__
#import <Carbon/Carbon.h>
#endif  // __APPLE__

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(init, __VA_ARGS__)
#define I(...) printf(__VA_ARGS__)

namespace android {
namespace crashreport {

using ::android::base::LazyInstance;
using ::android::base::PathUtils;
using ::android::base::RunOptions;
using ::android::base::StringFormat;
using ::android::base::System;

namespace {

const char kCrashURL_prod[] = "https://clients2.google.com/cr/report";
const char kCrashURL_staging[] =
        "https://clients2.google.com/cr/staging_report";

#if defined(_WIN32)
const char kPipeName[] = "\\\\.\\pipe\\AndroidEmulatorCrashService";
#elif defined(__APPLE__)
const char kPipeName[] = "com.google.AndroidEmulator.CrashService";
#endif

const char kCrashSubDir[] = "breakpad";

class HostCrashSystem : public CrashSystem {
public:
    HostCrashSystem() = default;

    virtual ~HostCrashSystem() = default;

    virtual const std::string& getCrashDirectory() override {
        if (mCrashDir.empty()) {
            mCrashDir =
                    PathUtils::join(::android::ConfigDirs::getUserDirectory(),
                                    kCrashSubDir)
                            .c_str();
        }
        return mCrashDir;
    }

    virtual const std::string& getCrashURL() override {
        if (mCrashURL.empty()) {
            if (CrashType::CRASHUPLOAD == CrashType::PROD) {
                mCrashURL.assign(kCrashURL_prod);
            } else if (CrashType::CRASHUPLOAD == CrashType::STAGING) {
                mCrashURL.assign(kCrashURL_staging);
            }
        }
        return mCrashURL;
    }

    virtual bool validatePaths() override {
        bool valid = CrashSystem::validatePaths();
        std::string crashDir = getCrashDirectory();
        if (!System::get()->pathExists(crashDir)) {
            if (path_mkdir_if_needed(crashDir.c_str(), 0744) == -1) {
                W("Couldn't create crash dir %s\n", crashDir.c_str());
                valid = false;
            }
        } else if (System::get()->pathIsFile(crashDir)) {
            W("Crash dir is a file! %s\n", crashDir.c_str());
            valid = false;
        }
        return valid;
    }

private:
    std::string mCrashDir;
    std::string mCrashURL;
};

LazyInstance<HostCrashSystem> sCrashSystem = LAZY_INSTANCE_INIT;
CrashSystem* sCrashSystemForTesting = nullptr;

}  // namespace

int CrashSystem::spawnService(const std::vector<std::string>& commandLine) {
    System::Pid pid;
    auto success = System::get()->runCommand(commandLine, RunOptions::Default,
                                             System::kInfinite, nullptr, &pid);
    return success ? pid : -1;
}

const std::string& CrashSystem::getCaBundlePath() {
    if (mCaBundlePath.empty()) {
        mCaBundlePath = PathUtils::join(System::get()->getLauncherDirectory(),
                                        "lib", "ca-bundle.pem")
                                .c_str();
    }
    return mCaBundlePath;
}

const std::string& CrashSystem::getCrashServicePath() {
    if (mCrashServicePath.empty()) {
        mCrashServicePath =
                PathUtils::join(
                        System::get()->getLauncherDirectory(),
                        PathUtils::toExecutableName(StringFormat(
                                "emulator%s-crash-service",
                                System::kProgramBitness == 64 ? "64" : "")))
                        .c_str();
    }
    return mCrashServicePath;
}

const std::string& CrashSystem::getProcessId() {
    if (mProcessId.empty()) {
#if defined(__linux__) || defined(__APPLE__)
        mProcessId = std::to_string(getpid());
#elif defined(_WIN32)
        mProcessId = std::to_string(GetCurrentProcessId());
#endif
    }
    return mProcessId;
}

const CrashSystem::CrashPipe& CrashSystem::getCrashPipe() {
    if (!mCrashPipe.isValid()) {
#if defined(__linux__)
        int server_fd;
        int client_fd;
        if (!google_breakpad::CrashGenerationServer::CreateReportChannel(
                    &server_fd, &client_fd)) {
            W("CrashReporter CreateReportChannel failed!\n");
        } else {
            mCrashPipe = CrashPipe(server_fd, client_fd);
        }
#elif defined(__APPLE__) || defined(_WIN32)
        std::string pipename;
        pipename.append(kPipeName);
        pipename.append(".");
        pipename.append(getProcessId());
        mCrashPipe = CrashPipe(pipename);
#endif
    }
    return mCrashPipe;
}

std::vector<std::string> CrashSystem::getCrashServiceCmdLine(
        const std::string& pipe,
        const std::string& proc) {
    const std::vector<std::string> cmdline = {
            getCrashServicePath(),
            "-pipe",
            pipe,
            "-ppid",
            proc,
            "-data-dir",
            CrashReporter::get()->getDataExchangeDir()};
    return cmdline;
}

bool CrashSystem::validatePaths() {
    bool valid = true;
    std::string caBundlePath = getCaBundlePath();
    std::string crashServicePath = getCrashServicePath();
    if (!System::get()->pathIsFile(caBundlePath)) {
        W("Couldn't find file %s\n", caBundlePath.c_str());
        valid = false;
    }
    if (!System::get()->pathIsFile(crashServicePath)) {
        W("Couldn't find crash service executable %s\n",
          crashServicePath.c_str());
        valid = false;
    }
    return valid;
}

CrashSystem* CrashSystem::get() {
    CrashSystem* result = sCrashSystemForTesting;
    if (!result) {
        result = sCrashSystem.ptr();
    }
    return result;
}

// static
bool CrashSystem::isDump(const std::string& path) {
    return PathUtils::extension(path) == ".dmp" &&
           System::get()->pathIsFile(path);
}

// static
CrashSystem* CrashSystem::setForTesting(CrashSystem* crashsystem) {
    CrashSystem* result = sCrashSystemForTesting;
    sCrashSystemForTesting = crashsystem;
    return result;
}

}  // namespace crashreport
}  // namespace android
