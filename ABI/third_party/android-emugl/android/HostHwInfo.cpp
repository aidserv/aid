// Copyright 2017 The Android Open Source Project
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

#include "android/HostHwInfo.h"

#include "android/base/memory/LazyInstance.h"
#include "android/base/StringView.h"
#include "android/base/system/System.h"
#include "android/cpu_accelerator.h"
#include "android/emulation/CpuAccelerator.h"
#include "android/utils/x86_cpuid.h"

#define DEBUG 0

#if DEBUG

#define D(fmt,...) do { \
    fprintf(stderr, "%s: " fmt "\n", __func__, ##__VA_ARGS__); \
} while(0) \

#else

#define D(...)

#endif

using android::base::LazyInstance;
using android::base::OsType;
using android::base::StringView;
using android::base::System;

namespace android {

static LazyInstance<HostHwInfo> sHostHwInfo =
    LAZY_INSTANCE_INIT;

HostHwInfo::HostHwInfo() {
    D("start");

    AndroidCpuInfoFlags cpuFlags =
        android::GetCpuInfo().first;

    StringView cpu_manufacturer_str = "Unknown";
    if (cpuFlags | ANDROID_CPU_INFO_AMD)
        cpu_manufacturer_str = "AMD";
    if (cpuFlags | ANDROID_CPU_INFO_INTEL)
        cpu_manufacturer_str = "Intel";

    uint32_t model_family_stepping;
    android_get_x86_cpuid(1, 0, &model_family_stepping,
                          nullptr, nullptr, nullptr);

    info.cpu_manufacturer = cpu_manufacturer_str;
    info.virt_support = cpuFlags & ANDROID_CPU_INFO_VIRT_SUPPORTED;
    info.running_in_vm = cpuFlags & ANDROID_CPU_INFO_VM;
    info.os_bit_count = System::get()->getHostBitness();
    info.cpu_model_name = model_family_stepping;
    info.os_platform = android::base::toString(System::get()->getOsType());
    info.gpuinfolist = &globalGpuInfoList();

    D("cpu info:\n"
      "    manufacturer %s\n"
      "    virt support %d invm %d\n"
      "    osbitcount %d\n"
      "    cpumodel 0x%x\n"
      "    osplatform %s",
      info.cpu_manufacturer.c_str(),
      info.virt_support,
      info.running_in_vm,
      info.os_bit_count,
      info.cpu_model_name,
      info.os_platform.c_str());

    for (size_t i = 0; i < info.gpuinfolist->infos.size(); i++) {
        const GpuInfo& gpuinfo = info.gpuinfolist->infos[i];
        D("gpu %zu:\n"
          "    make %s\n"
          "    model %s\n"
          "    device_id %s\n"
          "    revision_id %s\n"
          "    version %s\n"
          "    renderer %s\n",
          i, gpuinfo.make.c_str(), gpuinfo.model.c_str(),
          gpuinfo.device_id.c_str(), gpuinfo.revision_id.c_str(),
          gpuinfo.version.c_str(), gpuinfo.renderer.c_str());
        (void)gpuinfo;
    }
}

// static
HostHwInfo* HostHwInfo::get() {
    return sHostHwInfo.ptr();
}

// static
HostHwInfo::Info HostHwInfo::query() {
    return sHostHwInfo.get().info;
}

} // namespace android
