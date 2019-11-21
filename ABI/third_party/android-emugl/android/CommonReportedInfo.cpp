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

#include "android/CommonReportedInfo.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/Lock.h"

#include "android/metrics/proto/studio_stats.pb.h"

using android::base::AutoLock;
using android::base::Lock;
using android::base::LazyInstance;

namespace android {
namespace CommonReportedInfo {

class AllCommonInfo {
public:
    AllCommonInfo() = default;

    android_studio::EmulatorHost hostinfo;
    android_studio::EmulatorDetails details;
    android_studio::EmulatorPerformanceStats performanceStats;
    Lock lock;
    DISALLOW_COPY_AND_ASSIGN(AllCommonInfo);
};

static LazyInstance<AllCommonInfo> sCommonInfo;

static AllCommonInfo* get() {
    return sCommonInfo.ptr();
}

void setHostInfo(const android_studio::EmulatorHost* hostinfo) {
    AutoLock lock(get()->lock);
    get()->hostinfo = *hostinfo;
}

void setDetails(const android_studio::EmulatorDetails* details) {
    AutoLock lock(get()->lock);
    get()->details = *details;
}

void setPerformanceStats(const android_studio::EmulatorPerformanceStats* stats) {
    AutoLock lock(get()->lock);
    get()->performanceStats = *stats;
}

void appendMemoryUsage() {
    AutoLock lock(get()->lock);

    android_studio::EmulatorMemoryUsage* memUsageProto =
        get()->performanceStats.add_memory_usage();

    base::System::MemUsage usage = base::System::get()->getMemUsage();
    memUsageProto->set_resident_memory(usage.resident);
    memUsageProto->set_resident_memory_max(usage.resident_max);
    memUsageProto->set_virtual_memory(usage.virt);
    memUsageProto->set_virtual_memory_max(usage.virt_max);
    memUsageProto->set_total_phys_memory(usage.total_phys_memory);
    memUsageProto->set_total_page_file(usage.total_page_file);
}

void setUptime(base::System::Duration uptime) {
    AutoLock lock(get()->lock);
    get()->details.set_wall_time((uint64_t)uptime);
}

void setSessionPhase(AndroidSessionPhase phase) {
    AutoLock lock(get()->lock);
    // AndroidSessionPhase has same bits as
    // the proto's android_studio::EmulatorDetails::EmulatorSessionPhase,
    // at least for now.
    get()->details.set_session_phase(
        (android_studio::EmulatorDetails::EmulatorSessionPhase)phase);
}

void writeHostInfo(std::string* res) {
    AutoLock lock(get()->lock);
    get()->hostinfo.SerializeToString(res);
}

void writeDetails(std::string* res) {
    AutoLock lock(get()->lock);
    get()->details.SerializeToString(res);
}

void writePerformanceStats(std::string* res) {
    AutoLock lock(get()->lock);
    get()->performanceStats.SerializeToString(res);
}

void writeHostInfo(std::ostream* out) {
    AutoLock lock(get()->lock);
    get()->hostinfo.SerializeToOstream(out);
}

void writeDetails(std::ostream* out) {
    AutoLock lock(get()->lock);
    get()->details.SerializeToOstream(out);
}

void writePerformanceStats(std::ostream* out) {
    AutoLock lock(get()->lock);
    get()->performanceStats.SerializeToOstream(out);
}

void readHostInfo(const std::string& str) {
    AutoLock lock(get()->lock);
    get()->hostinfo.ParseFromString(str);
}

void readDetails(const std::string& str) {
    AutoLock lock(get()->lock);
    get()->details.ParseFromString(str);
}

void readPerformanceStats(const std::string& str) {
    AutoLock lock(get()->lock);
    get()->performanceStats.ParseFromString(str);
}

void readHostInfo(std::istream* in) {
    AutoLock lock(get()->lock);
    get()->hostinfo.ParseFromIstream(in);
}

void readDetails(std::istream* in) {
    AutoLock lock(get()->lock);
    get()->details.ParseFromIstream(in);
}

void readPerformanceStats(std::istream* in) {
    AutoLock lock(get()->lock);
    get()->performanceStats.ParseFromIstream(in);
}

} // namespace CommonReportedInfo
} // namespace android
