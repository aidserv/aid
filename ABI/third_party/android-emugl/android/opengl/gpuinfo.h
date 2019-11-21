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
#pragma once

#include "android/base/Compiler.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <vector>

// gpuinfo is designed to collect information about the GPUs
// installed on the host system for the purposes of
// automatically determining which renderer to select,
// in the cases where the GPU drivers are known to have issues
// running the emulator.

// The main entry points:

// load_gpu_info() queries the host system
// and returns a platform-specific string
// describing all GPU's on the system.
std::string load_gpu_info();

// host_gpu_blacklisted_async() does the two steps above,
// but on a different thread, with a timeout in case
// the started processes hang or what not.
bool async_query_host_gpu_blacklisted();
bool async_query_host_gpu_AngleWhitelisted();
bool async_query_host_gpu_SyncBlacklisted();

// Below is the implementation.

struct GpuInfoView{
    const char* make;
    const char* model;
    const char* device_id;
    const char* revision_id;
    const char* version;
    const char* renderer;
    const char* os;
};
// We keep a blacklist of known crashy GPU drivers
// as a static const list with items of this type:
using BlacklistEntry = GpuInfoView;
// We keep a whitelist to use Angle for buggy
// GPU drivers
using WhitelistEntry = GpuInfoView;

// GpuInfo/GpuInfoList are the representation of parsed information
// about the system's GPU.s
class GpuInfo {
public:
    GpuInfo() : current_gpu(false) { }
    GpuInfo(const std::string& _make,
            const std::string& _model,
            const std::string& _device_id,
            const std::string& _revision_id,
            const std::string& _version,
            const std::string& _renderer) :
        current_gpu(false),
        make(_make),
        model(_model),
        device_id(_device_id),
        revision_id(_revision_id),
        version(_version),
        renderer(_renderer) { }

    bool current_gpu;

    void addDll(std::string dll_str);

    std::string make;
    std::string model;
    std::string device_id;
    std::string revision_id;
    std::string version;
    std::string renderer;

    std::vector<std::string> dlls;
    std::string os;
};

class GpuInfoList {
public:
    GpuInfoList() = default;
    void addGpu();
    GpuInfo& currGpu();
    std::string dump() const;
    void clear();

    std::vector<GpuInfo> infos;

    bool blacklist_status = false;
    bool Anglelist_status = false;
    bool SyncBlacklist_status = false;

    DISALLOW_COPY_ASSIGN_AND_MOVE(GpuInfoList);
};

// Below are helper functions that can be useful in various
// contexts (e.g., unit testing).

// gpuinfo_query_blacklist():
// Function to query a given blacklist of GPU's.
// The blacklist |list| (of length |size|) attempts
// to match all non-NULL entry fields exactly against
// info of all GPU's in |gpulist|. If there is any match,
// the host system is considered on the blacklist.
// (Null blacklist entry fields are ignored and
// essentially act as wildcards).
bool gpuinfo_query_blacklist(GpuInfoList* gpulist,
                             const BlacklistEntry* list,
                             int size);

// Main function to parse GPU information.
void parse_gpu_info_list(const std::string& contents, GpuInfoList* gpulist);

// Platform-specific information parsing functions.
void parse_gpu_info_list_osx(const std::string& contents, GpuInfoList* gpulist);
void parse_gpu_info_list_linux(const std::string& contents, GpuInfoList* gpulist);
void parse_gpu_info_list_windows(const std::string& contents, GpuInfoList* gpulist);

// If we actually switched to software, call this.
void setGpuBlacklistStatus(bool switchedToSoftware);

// Return a fully loaded global GPU info list.
const GpuInfoList& globalGpuInfoList();
