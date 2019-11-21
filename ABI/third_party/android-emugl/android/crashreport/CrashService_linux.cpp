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

#include "android/crashreport/CrashService_linux.h"

#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"

#include "android/crashreport/CrashService.h"

#include "android/crashreport/CrashSystem.h"
#include "android/utils/debug.h"
#include "android/utils/path.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <memory>

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(init, __VA_ARGS__)
#define I(...) printf(__VA_ARGS__)

#define HWINFO_CMD "lshw"

namespace android {

using ::android::base::PathUtils;
using ::android::base::System;

namespace crashreport {

HostCrashService::~HostCrashService() {
    stopCrashServer();
}

void HostCrashService::OnClientDumpRequest(
        void* context,
        const ::google_breakpad::ClientInfo* client_info,
        const ::std::string* file_path) {
    if (static_cast<CrashService::DumpRequestContext*>(context)
                ->file_path.empty()) {
        D("Client Requesting dump %s\n", file_path->c_str());
        static_cast<CrashService::DumpRequestContext*>(context)->file_path =
                *file_path;
    }
}

void HostCrashService::OnClientExit(
        void* context,
        const ::google_breakpad::ClientInfo* client_info) {
    D("Client exiting\n");
    CrashService::ServerState* serverstate =
            static_cast<CrashService::ServerState*>(context);
    if (serverstate->connected > 0) {
        serverstate->connected -= 1;
    }
    if (serverstate->connected == 0) {
        serverstate->waiting = false;
    }
}

bool HostCrashService::startCrashServer(const std::string& pipe) {
    const int listen_fd = std::stoi(pipe);
    if (mCrashServer) {
        return false;
    }

    initCrashServer();

    mCrashServer.reset(new ::google_breakpad::CrashGenerationServer(
            listen_fd, OnClientDumpRequest, &mDumpRequestContext, OnClientExit,
            &mServerState, true, &CrashSystem::get()->getCrashDirectory()));

    return mCrashServer->Start();
}

bool HostCrashService::stopCrashServer() {
    if (mCrashServer) {
        mCrashServer.reset();
        return true;
    } else {
        return false;
    }
}

bool HostCrashService::isClientAlive() {
    if (!mClientPID) {
        return 0;
    }
    // waitpid for child clients
    waitpid(mClientPID, nullptr, WNOHANG);
    // kill with 0 signal will return non 0 if process does not exist
    if (kill(mClientPID, 0) != 0) {
        return false;
    } else {
        return true;
    }
}

bool HostCrashService::getHWInfo() {
    std::string data_directory = getDataDirectory();
    if (data_directory.empty()) {
        E("Unable to get data directory for crash report attachments");
        return false;
    }
    std::string file_path = PathUtils::join(data_directory, kHwInfoName);

    std::string syscmd(HWINFO_CMD);
    syscmd += " > ";
    syscmd += file_path;
    fprintf(stderr, "Running '%s' to get hardware info", syscmd.c_str());
    int status = system(syscmd.c_str());

    if (status != 0) {
        E("Unable to get hardware info for crash report");
        return false;
    }
    return true;
}

bool HostCrashService::getMemInfo() {
    // /proc/meminfo contains all the details we need so just upload that
    std::string file_path = PathUtils::join(getDataDirectory(), kMemInfoName);
    return path_copy_file_safe(file_path.c_str(), "/proc/meminfo") == 0;
}

}  // namespace crashreport
}  // namespace android
