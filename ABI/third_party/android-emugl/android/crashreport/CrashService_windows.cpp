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

#include "android/crashreport/CrashService_windows.h"

#include "android/base/files/PathUtils.h"
#include "android/base/StringFormat.h"
#include "android/base/StringView.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/base/system/System.h"

#include "android/base/system/Win32UnicodeString.h"
#include "android/crashreport/CrashReporter.h"
#include "android/crashreport/CrashSystem.h"
#include "android/utils/debug.h"

#include <fstream>
#include <string>

#include <sys/types.h>
#include <unistd.h>

#include <psapi.h>

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(init, __VA_ARGS__)
#define I(...) printf(__VA_ARGS__)

namespace android {

using ::android::base::PathUtils;
using ::android::base::StringView;
using ::android::base::System;
using ::android::base::Win32UnicodeString;

namespace crashreport {

HostCrashService::~HostCrashService() {
    stopCrashServer();
}

void HostCrashService::OnClientConnect(
        void* context,
        const google_breakpad::ClientInfo* client_info) {
    D("Client connected, pid = %d\n", client_info->pid());
    static_cast<CrashService::ServerState*>(context)->connected += 1;
}

void HostCrashService::OnClientDumpRequest(
        void* context,
        const google_breakpad::ClientInfo* client_info,
        const std::wstring* file_path) {
    if (static_cast<CrashService::DumpRequestContext*>(context)
                ->file_path.empty()) {
        ::std::string file_path_string =
                Win32UnicodeString::convertToUtf8(
                        file_path->c_str());
        D("Client Requesting dump %s\n", file_path_string.c_str());
        static_cast<CrashService::DumpRequestContext*>(context)
                ->file_path.assign(file_path_string.c_str());
    }
}

void HostCrashService::OnClientExit(
        void* context,
        const google_breakpad::ClientInfo* client_info) {
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
    if (mCrashServer) {
        return false;
    }

    initCrashServer();

    Win32UnicodeString pipe_unicode(pipe.c_str(), pipe.length());
    ::std::wstring pipe_string(pipe_unicode.data());
    Win32UnicodeString crashdir_unicode(
            ::android::crashreport::CrashSystem::get()
                    ->getCrashDirectory()
                    .c_str());
    std::wstring crashdir_wstr(crashdir_unicode.c_str());

    mCrashServer.reset(new ::google_breakpad::CrashGenerationServer(
            pipe_string, nullptr, OnClientConnect, &mServerState,
            OnClientDumpRequest, &mDumpRequestContext, OnClientExit,
            &mServerState, nullptr, nullptr, true, &crashdir_wstr));

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

bool HostCrashService::setClient(int clientpid) {
    mClientProcess.reset(OpenProcess(SYNCHRONIZE, FALSE, clientpid));
    return mClientProcess.get() != nullptr;
}

bool HostCrashService::isClientAlive() {
    if (!mClientProcess) {
        return false;
    }
    if (WaitForSingleObject(mClientProcess.get(), 0) != WAIT_TIMEOUT) {
        return false;
    } else {
        return true;
    }
}

bool HostCrashService::getHWInfo() {
    const std::string& dataDirectory = getDataDirectory();
    if (dataDirectory.empty()) {
        E("Unable to get data directory for crash report attachments");
        return false;
    }
    std::string utf8Path = PathUtils::join(dataDirectory, kHwInfoName);
    {
#if defined(_WIN32) && !defined(_WIN64)
        // Disable the 32-bit file system redirection to make sure we run the
        // 64-bit dxdiag here: "dxdiag /64bit" from a 32-bit process launches
        // x64 version asynchronously, and that's not what we need.
        PVOID oldValue;
        ::Wow64DisableWow64FsRedirection(&oldValue);
        const auto redirectionRestore = android::base::makeCustomScopedPtr(
                oldValue,
                [](PVOID oldValue) { ::Wow64RevertWow64FsRedirection(oldValue); });
#endif
        if (!System::get()->runCommand(
                    {"dxdiag", "/dontskip", "/whql:off", "/t", utf8Path},
                    System::RunOptions::WaitForCompletion, System::kInfinite)){
            E("dxdiag command failed to run");
            std::ofstream out(utf8Path.c_str());
            out << "dxdiag command failed to run\n";
            return false;
        }
    }

    if (!System::get()->pathExists(utf8Path)) {
        E("No output file from dxdiag command");
        std::ofstream out(utf8Path.c_str());
        out << "No output file from dxdiag command\n";
        return false;
    }

    // Sometimes dxdiag doesn't close the output file fast enough, and emulator
    // fails to read it. Make sure it's readable before continuing to fix it.
    const int kMaxWaitTimeUs = 1 * 1000 * 1000;
    const auto now = System::get()->getHighResTimeUs();
    do {
        if (System::get()->pathCanRead(utf8Path)) {
            std::ifstream in(utf8Path.c_str());
            if (in) {
                return true;
            }
        }
        System::get()->sleepMs(50);
    } while (System::get()->getHighResTimeUs() < now + kMaxWaitTimeUs);

    E("Unable to read output file");
    std::ofstream out(utf8Path.c_str());
    out << "Unable to read output file\n";
    return false;
}

// Convenience function to convert a value to a value in kilobytes
static uint64_t toKB(uint64_t value) {
    return value / 1024;
}

bool HostCrashService::getMemInfo() {
    const std::string& data_directory = getDataDirectory();
    if (data_directory.empty()) {
        E("Unable to get data directory for crash report attachments");
        return false;
    }
    std::string path = PathUtils::join(data_directory, kMemInfoName);
    // TODO: Replace ofstream when we have a good way of handling UTF-8 paths
    std::ofstream fout(path.c_str());
    if (!fout) {
        E("Unable to open '%s' to write crash report attachment", path.c_str());
        return false;
    }

    MEMORYSTATUSEX mem;
    mem.dwLength  = sizeof(mem);
    if (!GlobalMemoryStatusEx(&mem)) {
        DWORD error = GetLastError();
        E("Failed to get global memory status: %lu", error);
        fout << "ERROR: Failed to get global memory status: " << error << "\n";
        return false;
    }

    PERFORMANCE_INFORMATION pi = { sizeof(pi) };
    if (!GetPerformanceInfo(&pi, sizeof(pi))) {
        DWORD error = GetLastError();
        E("Failed to get performance info: %lu", error);
        fout << "ERROR: Failed to get performance info: " << error << "\n";
        return false;
    }

    size_t pageSize = pi.PageSize;
    fout << "Total physical memory: " << toKB(mem.ullTotalPhys) << " kB\n"
         << "Avail physical memory: " << toKB(mem.ullAvailPhys) << " kB\n"
         << "Total page file: " << toKB(mem.ullTotalPageFile) << " kB\n"
         << "Avail page file: " << toKB(mem.ullAvailPageFile) << " kB\n"
         << "Total virtual: " << toKB(mem.ullTotalVirtual) << " kB\n"
         << "Avail virtual: " << toKB(mem.ullAvailVirtual) << " kB\n"
         << "Commit total: " << toKB(pi.CommitTotal * pageSize) << " kB\n"
         << "Commit limit: " << toKB(pi.CommitLimit * pageSize) << " kB\n"
         << "Commit peak: " << toKB(pi.CommitPeak * pageSize) << " kB\n"
         << "System cache: " << toKB(pi.SystemCache * pageSize) << " kB\n"
         << "Kernel total: " << toKB(pi.KernelTotal * pageSize) << " kB\n"
         << "Kernel paged: " << toKB(pi.KernelPaged * pageSize) << " kB\n"
         << "Kernel nonpaged: " << toKB(pi.KernelNonpaged * pageSize) << " kB\n"
         << "Handle count: " << pi.HandleCount << "\n"
         << "Process count: " << pi.ProcessCount << "\n"
         << "Thread count: " << pi.ThreadCount << "\n";

    return fout.good();
}

void HostCrashService::collectProcessList()
{
    if (mDataDirectory.empty()) {
        return;
    }

    auto command = android::base::StringFormat(
                       "tasklist /V >%s\\%s",
                       mDataDirectory,
                       CrashReporter::kProcessListFileName);

    if (system(command.c_str()) != 0) {
        // try to call the "query process *" command, which used to exist
        // before the taskkill
        command = android::base::StringFormat(
                    "query process * >%s\\%s",
                    mDataDirectory,
                    CrashReporter::kProcessListFileName);
        system(command.c_str());
    }
}

}  // namespace crashreport
}  // namespace android
