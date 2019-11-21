// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/async/Looper.h"
#include "android/base/async/ThreadLooper.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/system/System.h"
#include "android/base/system/Win32UnicodeString.h"
#include "android/base/threads/ParallelTask.h"
#include "android/base/threads/Thread.h"
#include "android/opengl/gpuinfo.h"
#include "android/utils/file_io.h"

#include <algorithm>
#include <assert.h>
#include <inttypes.h>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using android::base::Looper;
using android::base::ParallelTask;
using android::base::RunOptions;
using android::base::StringView;
using android::base::System;
#ifdef _WIN32
using android::base::Win32UnicodeString;
#endif

// Try to switch to NVIDIA on Optimus systems,
// and AMD GPU on AmdPowerXpress.
// See http://developer.download.nvidia.com/devzone/devcenter/gamegraphics/files/OptimusRenderingPolicies.pdf
// and https://community.amd.com/thread/169965
// These variables need to be visible from the final emulator executable
// as exported symbols.
#ifdef _WIN32
#define FLAG_EXPORT __declspec(dllexport)
#else
#define FLAG_EXPORT __attribute__ ((visibility ("default")))
#endif

FLAG_EXPORT int NvOptimusEnablement = 0x00000001;
FLAG_EXPORT int AmdPowerXpressRequestHighPerformance = 0x00000001;

#undef FLAG_EXPORT

static const System::Duration kGPUInfoQueryTimeoutMs = 5000;
static const System::Duration kQueryCheckIntervalMs = 66;
static const size_t kFieldLen = 2048;

static const size_t NOTFOUND = std::string::npos;

static ::android::base::LazyInstance<GpuInfoList> sGpuInfoList =
        LAZY_INSTANCE_INIT;

void GpuInfo::addDll(std::string dll_str) {
    dlls.push_back(dll_str);
}

void GpuInfoList::addGpu() {
    infos.push_back(GpuInfo());
}
GpuInfo& GpuInfoList::currGpu() {
    if (infos.empty()) { addGpu(); }
    return infos.back();
}

std::string GpuInfoList::dump() const {
    std::stringstream ss;
    for (unsigned int i = 0; i < infos.size(); i++) {
        ss << "GPU #" << i + 1 << std::endl;

        if (!infos[i].make.empty()) {
            ss << "  Make: " << infos[i].make << std::endl;
        }
        if (!infos[i].model.empty()) {
            ss << "  Model: " << infos[i].model << std::endl;
        }
        if (!infos[i].device_id.empty()) {
            ss << "  Device ID: " << infos[i].device_id << std::endl;
        }
        if (!infos[i].revision_id.empty()) {
            ss << "  Revision ID: " << infos[i].revision_id << std::endl;
        }
        if (!infos[i].version.empty()) {
            ss << "  Driver version: " << infos[i].version << std::endl;
        }
        if (!infos[i].renderer.empty()) {
            ss << "  Renderer: " << infos[i].renderer << std::endl;
        }
    }
    return ss.str();
}

void GpuInfoList::clear() {
    blacklist_status = false;
    Anglelist_status = false;
    SyncBlacklist_status = false;
    infos.clear();
}

static bool gpuinfo_query_list(GpuInfoList* gpulist,
                             const BlacklistEntry* list,
                             int size) {
    for (auto gpuinfo : gpulist->infos) {
        for (int i = 0; i < size; i++) {
            auto bl_entry = list[i];
            const char* bl_make = bl_entry.make;
            const char* bl_model = bl_entry.model;
            const char* bl_device_id = bl_entry.device_id;
            const char* bl_revision_id = bl_entry.revision_id;
            const char* bl_version = bl_entry.version;
            const char* bl_renderer = bl_entry.renderer;
            const char* bl_os = bl_entry.os;

            if (bl_make && (gpuinfo.make != bl_make))
                continue;
            if (bl_model && (gpuinfo.model != bl_model))
                continue;
            if (bl_device_id && (gpuinfo.device_id != bl_device_id))
                continue;
            if (bl_revision_id && (gpuinfo.revision_id != bl_revision_id))
                continue;
            if (bl_version && (gpuinfo.revision_id != bl_version))
                continue;
            if (bl_renderer && (gpuinfo.renderer.find(bl_renderer) ==
                                NOTFOUND))
                continue;
            if (bl_os && (gpuinfo.os != bl_os))
                continue;
            return true;
        }
    }
    return false;
}

// Actual blacklist starts here.
// Most entries imported from Chrome blacklist.
static const BlacklistEntry sGpuBlacklist[] = {

        // Make | Model | DeviceID | RevisionID | DriverVersion | Renderer |
        // OS
        {nullptr, nullptr, "0x7249", nullptr, nullptr,
         nullptr, "M"},  // ATI Radeon X1900 on Mac
        {"8086", nullptr, nullptr, nullptr, nullptr,
         "Mesa", "L"},  // Linux, Intel, Mesa
        {"8086", nullptr, nullptr, nullptr, nullptr,
         "mesa", "L"},  // Linux, Intel, Mesa

        {"8086", nullptr, "27ae", nullptr, nullptr,
         nullptr, nullptr},  // Intel 945 Chipset
        {"1002", nullptr, nullptr, nullptr, nullptr, nullptr,
          "L"},  // Linux, ATI

        {nullptr, nullptr, "0x9583", nullptr, nullptr,
         nullptr, "M"},  // ATI Radeon HD2600 on Mac
        {nullptr, nullptr, "0x94c8", nullptr, nullptr,
         nullptr, "M"},  // ATI Radeon HD2400 on Mac

        {"NVIDIA (0x10de)", nullptr, "0x0324", nullptr, nullptr,
         nullptr, "M"},  // NVIDIA GeForce FX Go5200 (Mac)
        {"10DE", "NVIDIA GeForce FX Go5200", nullptr, nullptr, nullptr,
         nullptr, "W"},  // NVIDIA GeForce FX Go5200 (Win)
        {"10de", nullptr, "0324", nullptr, nullptr,
         nullptr, "L"},  // NVIDIA GeForce FX Go5200 (Linux)

        {"10de", nullptr, "029e", nullptr, nullptr,
         nullptr, "L"},  // NVIDIA Quadro FX 1500 (Linux)

        // Various Quadro FX cards on Linux
        {"10de", nullptr, "00cd", nullptr, "195.36.24",
          nullptr, "L"},
        {"10de", nullptr, "00ce", nullptr, "195.36.24",
         nullptr, "L"},
        // Driver version 260.19.6 on Linux
        {"10de", nullptr, nullptr, nullptr, "260.19.6",
         nullptr, "L"},

        {"NVIDIA (0x10de)", nullptr, "0x0393", nullptr, nullptr,
         nullptr, "M"},  // NVIDIA GeForce 7300 GT (Mac)
};

static const int sBlacklistSize =
    sizeof(sGpuBlacklist) / sizeof(BlacklistEntry);

// If any blacklist entry matches any gpu, return true.
bool gpuinfo_query_blacklist(GpuInfoList* gpulist,
                             const BlacklistEntry* list,
                             int size) {
    return gpuinfo_query_list(gpulist, list, size);
}

#ifdef _WIN32
static const WhitelistEntry sAngleWhitelist[] = {
        // Make | Model | DeviceID | RevisionID | DriverVersion | Renderer |
        // OS
        // HD 3000 on Windows
        {"8086", nullptr, "0116", nullptr, nullptr,
         nullptr, "W"},
        {"8086", nullptr, "0126", nullptr, nullptr,
         nullptr, "W"},
        {"8086", nullptr, "0102", nullptr, nullptr,
         nullptr, "W"},
};

static const int sAngleWhitelistSize =
    sizeof(sAngleWhitelist) / sizeof(WhitelistEntry);


static bool gpuinfo_query_whitelist(GpuInfoList *gpulist,
                             const WhitelistEntry *list,
                             int size) {
    return gpuinfo_query_list(gpulist, list, size);
}

#endif

static const BlacklistEntry sSyncBlacklist[] = {
    // Make | Model | DeviceID | RevisionID | DriverVersion | Renderer |
    // OS
    // All NVIDIA Quadro NVS and NVIDIA NVS GPUs on Windows
    {"10de", nullptr, "06fd", nullptr, nullptr, nullptr, "W"}, // NVS 295
    {"10de", nullptr, "0a6a", nullptr, nullptr, nullptr, "W"}, // NVS 2100M
    {"10de", nullptr, "0a6c", nullptr, nullptr, nullptr, "W"}, // NVS 5100M
    {"10de", nullptr, "0ffd", nullptr, nullptr, nullptr, "W"}, // NVS 510
    {"10de", nullptr, "1056", nullptr, nullptr, nullptr, "W"}, // NVS 4200M
    {"10de", nullptr, "10d8", nullptr, nullptr, nullptr, "W"}, // NVS 300
    {"10de", nullptr, "014a", nullptr, nullptr, nullptr, "W"}, // Quadro NVS 440
    {"10de", nullptr, "0165", nullptr, nullptr, nullptr, "W"}, // Quadro NVS 285
    {"10de", nullptr, "017a", nullptr, nullptr, nullptr, "W"}, // Quadro NVS (generic)
    {"10de", nullptr, "018a", nullptr, nullptr, nullptr, "W"}, // Quadro NVS AGP8X (generic)
    {"10de", nullptr, "018c", nullptr, nullptr, nullptr, "W"}, // Quadro NVS 50 PCI (generic)
    {"10de", nullptr, "01db", nullptr, nullptr, nullptr, "W"}, // Quadro NVS 120M
    {"10de", nullptr, "0245", nullptr, nullptr, nullptr, "W"}, // Quadro NVS 210S / NVIDIA GeForce 6150LE
    {"10de", nullptr, "032a", nullptr, nullptr, nullptr, "W"}, // Quadro NVS 55/280 PCI
    {"10de", nullptr, "040c", nullptr, nullptr, nullptr, "W"}, // Quadro NVS 570M / Mobile Quadro FX/NVS video card
    {"10de", nullptr, "0429", nullptr, nullptr, nullptr, "W"}, // Quadro NVS 135M or Quadro NVS 140M
    {"10de", nullptr, "042b", nullptr, nullptr, nullptr, "W"}, // Quadro NVS 135M
    {"10de", nullptr, "042f", nullptr, nullptr, nullptr, "W"}, // Quadro NVS 290
    {"10de", nullptr, "06ea", nullptr, nullptr, nullptr, "W"}, // quadro nvs 150m
    {"10de", nullptr, "06eb", nullptr, nullptr, nullptr, "W"}, // Quadro NVS 160M
    {"10de", nullptr, "06f8", nullptr, nullptr, nullptr, "W"}, // Quadro NVS 420
    {"10de", nullptr, "06fa", nullptr, nullptr, nullptr, "W"}, // Quadro NVS 450
    {"10de", nullptr, "0a2c", nullptr, nullptr, nullptr, "W"}, // Quadro NVS 5100M
};

static const int sSyncBlacklistSize =
    sizeof(sSyncBlacklist) / sizeof(BlacklistEntry);

static std::string readAndDeleteTempFile(StringView name) {
    std::ifstream fh(name.c_str(), std::ios::binary);
    if (!fh) {
#ifdef _WIN32
        Sleep(100);
#endif
        fh.open(name.c_str(), std::ios::binary);
        if (!fh) {
            return {};
        }
    }

    std::stringstream ss;
    ss << fh.rdbuf();
    std::string contents = ss.str();
    fh.close();

#ifndef _WIN32
    remove(name.c_str());
#else
    DWORD del_ret = DeleteFile(name.c_str());
    if (!del_ret) {
        Sleep(100);
        DeleteFile(name.c_str());
    }
#endif
    return contents;
}

std::string load_gpu_info() {
    auto& sys = *System::get();
    std::string tmp_dir = sys.getTempDir();

// Get temporary file path
#ifndef _WIN32
    if (tmp_dir.back() != '/') {
        tmp_dir += '/';
    }
    std::string temp_filename_pattern = "gpuinfo_XXXXXX";
    std::string temp_file_path_template = (tmp_dir + temp_filename_pattern);

    int tmpfd = mkstemp((char*)temp_file_path_template.data());

    if (tmpfd == -1) {
        return std::string();
    }

    StringView temp_file_path = temp_file_path_template.c_str();
#else
    char tmp_filename_buffer[kFieldLen] = {};
    DWORD temp_file_ret =
            GetTempFileName(tmp_dir.c_str(), "gpu", 0, tmp_filename_buffer);

    if (!temp_file_ret) {
        return {};
    }

    StringView temp_file_path = tmp_filename_buffer;
#endif
    std::string secondTempFile;

    // Execute the command to get GPU info.
#ifdef __APPLE__
    if (!sys.runCommand({"system_profiler", "SPDisplaysDataType"},
                        RunOptions::WaitForCompletion |
                        RunOptions::TerminateOnTimeout |
                        RunOptions::DumpOutputToFile,
                        kGPUInfoQueryTimeoutMs,
                        nullptr, nullptr, temp_file_path)) {
        return {};
    }
#elif !defined(_WIN32)
    if (!sys.runCommand({"lspci", "-mvnn"},
                        RunOptions::WaitForCompletion |
                        RunOptions::TerminateOnTimeout |
                        RunOptions::DumpOutputToFile,
                        kGPUInfoQueryTimeoutMs / 2,
                        nullptr, nullptr, temp_file_path)) {
        return {};
    }
    secondTempFile = std::string(temp_file_path) + ".glxinfo";
    if (!sys.runCommand({"glxinfo"},
                        System::RunOptions::WaitForCompletion |
                        System::RunOptions::TerminateOnTimeout |
                        System::RunOptions::DumpOutputToFile,
                        kGPUInfoQueryTimeoutMs / 2,
                        nullptr, nullptr, secondTempFile)) {
        return {};
    }
#else
    if (!sys.runCommand({"wmic", "/OUTPUT:" + std::string(temp_file_path),
                        "path", "Win32_VideoController", "get", "/value"},
                        RunOptions::WaitForCompletion |
                        RunOptions::TerminateOnTimeout,
                        kGPUInfoQueryTimeoutMs)) {
        return {};
    }
#endif

    std::string contents = readAndDeleteTempFile(temp_file_path);
    if (!secondTempFile.empty()) {
        contents += readAndDeleteTempFile(secondTempFile);
    }

#ifdef _WIN32
    // Windows outputs the data in UTF-16 encoding, let's convert it to the
    // common UTF-8.
    int num_chars = contents.size() / sizeof(wchar_t);
    contents = Win32UnicodeString::convertToUtf8(
            reinterpret_cast<const wchar_t*>(contents.c_str()), num_chars);
#endif

    return contents;
}

std::string parse_last_hexbrackets(const std::string& str) {
    size_t closebrace_p = str.rfind("]");
    size_t openbrace_p = str.rfind("[", closebrace_p - 1);
    return str.substr(openbrace_p + 1, closebrace_p - openbrace_p - 1);
}

std::string parse_renderer_part(const std::string& str) {
    size_t lastspace_p = str.rfind(" ");
    size_t prevspace_p = str.rfind(" ", lastspace_p - 1);
    return str.substr(prevspace_p + 1, lastspace_p - prevspace_p - 1);
}

void parse_gpu_info_list_osx(const std::string& contents,
                             GpuInfoList* gpulist) {
    size_t line_loc = contents.find("\n");
    if (line_loc == NOTFOUND) {
        line_loc = contents.size();
    }
    size_t p = 0;
    size_t kvsep_loc;
    std::string key;
    std::string val;

    // OS X: We expect a sequence of lines from system_profiler
    // that describe all GPU's connected to the system.
    // When a line containing
    //     Chipset Model: <gpu model name>
    // that's the earliest reliable indication of information
    // about an additional GPU. After that,
    // it's a simple matter to find the vendor/device ID lines.
    while (line_loc != NOTFOUND) {
        kvsep_loc = contents.find(": ", p);
        if ((kvsep_loc != NOTFOUND) && (kvsep_loc < line_loc)) {
            key = contents.substr(p, kvsep_loc - p);
            size_t valbegin = (kvsep_loc + 2);
            val = contents.substr(valbegin, line_loc - valbegin);
            if (key.find("Chipset Model") != NOTFOUND) {
                gpulist->addGpu();
                gpulist->currGpu().model = val;
                gpulist->currGpu().os = "M";
            } else if (key.find("Vendor") != NOTFOUND) {
                gpulist->currGpu().make = val;
            } else if (key.find("Device ID") != NOTFOUND) {
                gpulist->currGpu().device_id = val;
            } else if (key.find("Revision ID") != NOTFOUND) {
                gpulist->currGpu().revision_id = val;
            } else if (key.find("Display Type") != NOTFOUND) {
                gpulist->currGpu().current_gpu = true;
            } else {
            }
        }
        if (line_loc == contents.size()) {
            break;
        }
        p = line_loc + 1;
        line_loc = contents.find("\n", p);
        if (line_loc == NOTFOUND) {
            line_loc = contents.size();
        }
    }
}

void parse_gpu_info_list_linux(const std::string& contents,
                               GpuInfoList* gpulist) {
    size_t line_loc = contents.find("\n");
    if (line_loc == NOTFOUND) {
        line_loc = contents.size();
    }
    size_t p = 0;
    std::string key;
    std::string val;
    bool lookfor = false;

    // Linux - Only support one GPU for now.
    // On Linux, the only command that seems not to take
    // forever is lspci.
    // We just look for "VGA" in lspci, then
    // attempt to grab vendor and device information.
    // Second, we issue glxinfo and look for the version string,
    // in case there is a renderer such as Mesa
    // to look out for.
    while (line_loc != NOTFOUND) {
        key = contents.substr(p, line_loc - p);
        if (!lookfor && (key.find("VGA") != NOTFOUND)) {
            lookfor = true;
            gpulist->addGpu();
            gpulist->currGpu().os = "L";
        } else if (lookfor && (key.find("Vendor") != NOTFOUND)) {
            gpulist->currGpu().make = parse_last_hexbrackets(key);
        } else if (lookfor && (key.find("Device") != NOTFOUND)) {
            gpulist->currGpu().device_id = parse_last_hexbrackets(key);
            lookfor = false;
        } else if (key.find("OpenGL version string") != NOTFOUND) {
            gpulist->currGpu().renderer = key;
        } else {
        }
        if (line_loc == contents.size()) {
            break;
        }
        p = line_loc + 1;
        line_loc = contents.find("\n", p);
        if (line_loc == NOTFOUND) {
            line_loc = contents.size();
        }
    }
}

static void parse_windows_gpu_dlls(int line_loc, int val_pos,
                            const std::string& contents,
                            GpuInfoList* gpulist) {
    if (line_loc - val_pos == 0) {
        return;
    }

    const std::string& dll_str =
        contents.substr(val_pos, line_loc - val_pos);

    size_t vp = 0;
    size_t dll_sep_loc = dll_str.find(",", vp);
    size_t dll_end =
        (dll_sep_loc != NOTFOUND) ?  dll_sep_loc : dll_str.size() - vp;
    gpulist->currGpu().addDll(dll_str.substr(vp, dll_end - vp));

    while (dll_sep_loc != NOTFOUND) {
        vp = dll_sep_loc + 1;
        dll_sep_loc = dll_str.find(",", vp);
        dll_end =
            (dll_sep_loc != NOTFOUND) ?  dll_sep_loc : dll_str.size() - vp;
        gpulist->currGpu().addDll(
                dll_str.substr(vp, dll_end - vp));
    }

    const std::string& curr_make = gpulist->currGpu().make;
    if (curr_make == "NVIDIA") {
        gpulist->currGpu().addDll("nvoglv32.dll");
        gpulist->currGpu().addDll("nvoglv64.dll");
    }
    else if (curr_make == "Advanced Micro Devices, Inc.") {
        gpulist->currGpu().addDll("atioglxx.dll");
        gpulist->currGpu().addDll("atig6txx.dll");
    }
}

void parse_windows_gpu_ids(const std::string& val, GpuInfoList *gpulist) {
    std::string result;
    size_t key_start = 0;
    size_t key_end = 0;

    key_start = val.find("VEN_", key_start);
    if (key_start == NOTFOUND) {
        return;
    }
    key_end = val.find("&", key_start);
    if (key_end == NOTFOUND) {
        return;
    }
    result = val.substr(key_start + 4, key_end - key_start - 4);
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    gpulist->currGpu().make = result;

    key_start = val.find("DEV_", key_start);
    if (key_start == NOTFOUND) {
        return;
    }
    key_end = val.find("&", key_start);
    if (key_end == NOTFOUND) {
        return;
    }
    result = val.substr(key_start + 4, key_end - key_start - 4);
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    gpulist->currGpu().device_id = result;
}

void parse_gpu_info_list_windows(const std::string& contents,
                                 GpuInfoList* gpulist) {
    size_t line_loc = contents.find("\r\n");
    if (line_loc == NOTFOUND) {
        line_loc = contents.size();
    }
    size_t p = 0;
    size_t equals_pos = 0;
    size_t val_pos = 0;
    std::string key;
    std::string val;

    // Windows: We use `wmic path Win32_VideoController get /value`
    // to get a reasonably detailed list of '<key>=<val>'
    // pairs. From these, we can get the make/model
    // of the GPU, the driver version, and all DLLs involved.
   while (line_loc != NOTFOUND) {
        equals_pos = contents.find("=", p);
        if ((equals_pos != NOTFOUND) && (equals_pos < line_loc)) {
            key = contents.substr(p, equals_pos - p);
            val_pos = equals_pos + 1;
            val = contents.substr(val_pos, line_loc - val_pos);

            if (key.find("AdapterCompatibility") != NOTFOUND) {
                gpulist->addGpu();
                gpulist->currGpu().os = "W";
                // 'make' will be overwritten in parsing 'PNPDeviceID'
                // later. Set it here because we need it in paring
                // 'InstalledDisplayDrivers' which comes before
                // 'PNPDeviceID'.
                gpulist->currGpu().make = val;
            } else if (key.find("Caption") != NOTFOUND) {
                gpulist->currGpu().model = val;
            } else if (key.find("PNPDeviceID") != NOTFOUND) {
                parse_windows_gpu_ids(val, gpulist);
            }
            else if (key.find("DriverVersion") != NOTFOUND) {
                gpulist->currGpu().version = val;
            } else if (key.find("InstalledDisplayDrivers") != NOTFOUND) {
                parse_windows_gpu_dlls(line_loc, val_pos, contents, gpulist);
            }
        }
        if (line_loc == contents.size()) {
            break;
        }
        p = line_loc + 2;
        line_loc = contents.find("\r\n", p);
        if (line_loc == NOTFOUND) {
            line_loc = contents.size();
        }
    }
}

void parse_gpu_info_list(const std::string& contents, GpuInfoList* gpulist) {
#ifdef __APPLE__
    parse_gpu_info_list_osx(contents, gpulist);
#elif !defined(_WIN32)
    parse_gpu_info_list_linux(contents, gpulist);
#else
    parse_gpu_info_list_windows(contents, gpulist);
#endif
}

void query_blacklist_fn(bool* res) {
    std::string gpu_info = load_gpu_info();
    GpuInfoList* gpulist = sGpuInfoList.ptr();
    parse_gpu_info_list(gpu_info, gpulist);
    *res = gpuinfo_query_blacklist(gpulist, sGpuBlacklist, sBlacklistSize);
    sGpuInfoList->blacklist_status = *res;
#ifdef _WIN32
    sGpuInfoList->Anglelist_status =
        gpuinfo_query_whitelist(gpulist, sAngleWhitelist, sAngleWhitelistSize);
#else
    sGpuInfoList->Anglelist_status = false;
#endif
    sGpuInfoList->SyncBlacklist_status =
        gpuinfo_query_blacklist(gpulist, sSyncBlacklist, sSyncBlacklistSize);
}

void query_blacklist_done_fn(const bool& res) { }

// Separate thread for GPU info querying:
//
// Our goal is to account for circumstances where obtaining GPU info either
// takes too long or ties up the host system in a special way where the system
// ends up hanging. This is bad, since no progress will happen for emulator
// startup, which is more critical.
//
// We therefore use a ParallelTask and a looper with timeout to take care of
// this case.
//
// Note that we use a separate thread (rather than the main thread) because
// later on when skin_winsys_enter_main_loop is called, it will set the looper
// of the main thread to a custom Looper that works with Qt
// (android::qt::createLooper()).  Otherwise, creating a looper on the main
// thread at this point will prevent the custom looper from being used, which
// aborts the program.
class GPUInfoQueryThread : public android::base::Thread {
public:
    GPUInfoQueryThread() {
        this->start();
    }
    virtual intptr_t main() override {
        Looper* looper = android::base::ThreadLooper::get();
        android::base::runParallelTask<bool>
            (looper, &query_blacklist_fn, &query_blacklist_done_fn,
             kQueryCheckIntervalMs);
        looper->runWithTimeoutMs(kGPUInfoQueryTimeoutMs);
        looper->forceQuit();
        return 0;
    }
};

static android::base::LazyInstance<GPUInfoQueryThread> sGPUInfoQueryThread =
        LAZY_INSTANCE_INIT;

bool async_query_host_gpu_blacklisted() {
    return globalGpuInfoList().blacklist_status;
}

bool async_query_host_gpu_AngleWhitelisted() {
    return globalGpuInfoList().Anglelist_status;
}

bool async_query_host_gpu_SyncBlacklisted() {
    return globalGpuInfoList().SyncBlacklist_status;
}

void setGpuBlacklistStatus(bool switchedToSoftware) {
    sGpuInfoList->blacklist_status = switchedToSoftware;
}

const GpuInfoList& globalGpuInfoList() {
    sGPUInfoQueryThread->wait();
    return *sGpuInfoList;
}
