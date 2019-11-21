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

#include "android/crashreport/CrashService_darwin.h"

#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"

#include "android/crashreport/CrashSystem.h"
#include "android/utils/debug.h"

#include <mach/vm_statistics.h>
#include <mach/mach_types.h>
#include <mach/mach_init.h>
#include <mach/mach_host.h>
#include <sys/mount.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <fstream>
#include <memory>
#include <sstream>

#include <stdint.h>

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(init, __VA_ARGS__)
#define I(...) printf(__VA_ARGS__)

#define HWINFO_CMD "system_profiler -detailLevel mini"

namespace android {

using ::android::base::PathUtils;
using ::android::base::System;

namespace crashreport {

HostCrashService::~HostCrashService() {
    stopCrashServer();
}

void HostCrashService::OnClientDumpRequest(
        void* context,
        const google_breakpad::ClientInfo& client_info,
        const std::string& file_path) {
    if (static_cast<CrashService::DumpRequestContext*>(context)
                ->file_path.empty()) {
        D("Client Requesting dump %s\n", file_path.c_str());
        static_cast<CrashService::DumpRequestContext*>(context)->file_path =
                file_path;
    }
}

void HostCrashService::OnClientExit(
        void* context,
        const google_breakpad::ClientInfo& client_info) {
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

    mCrashServer.reset(new ::google_breakpad::CrashGenerationServer(
            pipe.c_str(), nullptr, nullptr, &OnClientDumpRequest,
            &mDumpRequestContext, &OnClientExit, &mServerState, true,
            CrashSystem::get()->getCrashDirectory()));
    if (!mCrashServer) {
        return false;
    }
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
        return false;
    }
    // waitpid added for child clients
    waitpid(mClientPID, nullptr, WNOHANG);
    // kill with 0 signal will return non 0 if process does not exist
    if (kill(mClientPID, 0) != 0) {
        return false;
    } else {
        return true;
    }
}

bool HostCrashService::getHWInfo() {
    std::string file_path = PathUtils::join(getDataDirectory(), kHwInfoName);

    std::string syscmd(HWINFO_CMD);
    syscmd += " > ";
    syscmd += file_path;
    int status = system(syscmd.c_str());

    if (status != 0) {
        E("Unable to get hardware info");
        return false;
    }
    return true;
}

// Convenience function to convert a value to a value in kilobytes
template<typename T>
static T toKB(T value) {
    return value / 1024;
}

bool HostCrashService::getMemInfo() {
    const std::string& data_directory = getDataDirectory();
    if (data_directory.empty()) {
        E("Unable to get data directory for crash report attachments");
        return false;
    }
    std::string file_path = PathUtils::join(data_directory, kMemInfoName);
    // Open this file early so we can print any errors into it, we might not
    // be able to get all data but whatever we can get is interesting and
    // knowing what failed could also be useful
    std::ofstream fout(file_path.c_str());
    if (!fout) {
        E("Unable to open '%s' file for writing", file_path.c_str());
        return false;
    }

    // Get total physical memory from the sysctl value "hw.memsize"
    uint64_t physicalMem = 0;
    size_t size = sizeof(physicalMem);
    if (sysctlbyname("hw.memsize", &physicalMem, &size, nullptr, 0) != 0) {
        E("Unable to get memory size");
        fout << "ERROR: Unable to get memory size: " << strerror(errno) << "\n";
    }

    // Determine page size, we're going to need it for VM stats
    mach_port_t machPort = mach_host_self();
    vm_size_t pageSize = 0;
    vm_statistics64_data_t vmStats;
    mach_msg_type_number_t count = sizeof(vmStats) / sizeof(integer_t);
    kern_return_t result = host_page_size(machPort, &pageSize);
    if (result != KERN_SUCCESS) {
        E("Unable to get page size");
        fout << "ERROR: Unable to get page size: " << result << "\n";
    }

    // Get host statistics which includes information about used/free mem
    result = host_statistics64(machPort, HOST_VM_INFO,
                               reinterpret_cast<host_info64_t>(&vmStats),
                               &count);
    if (result != KERN_SUCCESS) {
        E("Unable to get host statistics");
        fout << "ERROR: Unable to get host statistics: " << result << "\n";
    }
    uint64_t freeMem = vmStats.free_count * pageSize;
    uint64_t activeMem = vmStats.active_count * pageSize;
    uint64_t inactiveMem = vmStats.inactive_count * pageSize;
    uint64_t wiredMem = vmStats.wire_count * pageSize;

    // The largest possible swap is determined by the amount of space
    // available on the root filesystem. The Darwin swap can grow to
    // consume all that space.
    struct statfs stats;
    if (statfs("/", &stats) != 0) {
        E("Unale to stat root filesystem");
        fout << "ERROR: Unable to stat root filesystem: " << strerror(errno)
             << "\n";
    }
    uint64_t maxSwap = stats.f_bsize * stats.f_bfree;

    // Get swap details using the sysctl value "vm.swapusage". Note that
    // the total swap returned here is the current size of the swap file.
    // The swap file can grow as needed up to the root filesystem space
    xsw_usage vmUsage = {0};
    size = sizeof(vmUsage);
    if (sysctlbyname("vm.swapusage", &vmUsage, &size, nullptr, 0) != 0) {
        E("Unable to get swap usage");
        fout << "ERROR: Unable to get swap usage: " << strerror(errno) << "\n";
    }

    fout << "Physical memory: " << toKB(physicalMem) << " kB\n"
         << "Free memory: " << toKB(freeMem) << " kB\n"
         << "Active memory: " << toKB(activeMem) << " kB\n"
         << "Inactive memory: " << toKB(inactiveMem) << " kB\n"
         << "Wired memory: " << toKB(wiredMem) << " kB\n"
         << "Maximum possible swap: " << toKB(maxSwap) << " kB\n"
         << "Total swap: " << toKB(vmUsage.xsu_total) << " kB\n"
         << "Available swap: " << toKB(vmUsage.xsu_avail) << " kB\n"
         << "Used swap: " << toKB(vmUsage.xsu_used) << " kB\n";

    return fout.good();
}


}  // namespace crashreport
}  // namespace android
