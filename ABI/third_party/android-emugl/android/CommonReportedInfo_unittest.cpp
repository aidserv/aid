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

#include "android/base/files/ScopedFd.h"
#include "android/base/misc/FileUtils.h"
#include "android/utils/eintr_wrapper.h"
#include "android/utils/path.h"
#include "android/utils/tempfile.h"

#include "android/metrics/proto/studio_stats.pb.h"
#include "google/protobuf/text_format.h"

#include <gtest/gtest.h>

#include <fstream>

namespace android {

static const char kEmulatorHostTestStr[] = R"(
cpu_manufacturer: "INTEL"
virt_support: true
running_in_vm: false
os_bit_count: 64
cpuid_stepping: 2
cpuid_model: 15
cpuid_family: 6
cpuid_type: 0
cpuid_extmodel: 3
cpuid_extfamily: 0
)";

static const char kEmulatorDetailsTestStr[] = R"(
guest_arch: X86
guest_api_level: 26
guest_gpu_enabled: true
is_opengl_alive: true
guest_gl {
  vendor: "NVIDIA Corporation"
  renderer: "Quadro K2200/PCIe/SSE2"
  version: "4.5.0 NVIDIA 367.57"
}
host_gpu {
  make: "10de"
  model: ""
  device_id: "13ba"
  revision_id: ""
  version: ""
  renderer: "OpenGL version string: 4.5.0 NVIDIA 367.57"
}
session_phase: RUNNING_GENERAL
renderer: HOST
avd_info {
  api_level: 26
  arch: X86
  creation_timestamp: 1491510422
  build_id: "test_build_id"
  build_timestamp: 1492869301
  image_kind: AOSP
  files {
    kind: KERNEL
    location: STANDARD
    size: 5081264
  }
  files {
    kind: SYSTEM
    location: STANDARD
    size: 2684354560
  }
  files {
    kind: RAMDISK
    location: STANDARD
    size: 2173952
  }
  properties: PHONE_AVD
}
feature_flag_state {
  attempted_enabled_feature_flags: GL_PIPE_CHECKSUM
  attempted_enabled_feature_flags: ENCRYPT_USER_DATA
  attempted_enabled_feature_flags: INTEL_PERFORMANCE_MONITORING_UNIT
  attempted_enabled_feature_flags: GLDMA
  attempted_enabled_feature_flags: LOGCAT_PIPE
  user_overridden_enabled_features: ENCRYPT_USER_DATA
  user_overridden_enabled_features: GL_ASYNC_SWAP
  user_overridden_enabled_features: GLDMA
  user_overridden_enabled_features: GLES_DYNAMIC_VERSION
  user_overridden_enabled_features: PLAY_STORE_IMAGE
  user_overridden_disabled_features: GL_PIPE_CHECKSUM
  user_overridden_disabled_features: GRALLOC_SYNC
  resulting_enabled_features: ENCRYPT_USER_DATA
  resulting_enabled_features: INTEL_PERFORMANCE_MONITORING_UNIT
  resulting_enabled_features: GL_ASYNC_SWAP
  resulting_enabled_features: GLDMA
  resulting_enabled_features: GLES_DYNAMIC_VERSION
  resulting_enabled_features: PLAY_STORE_IMAGE
  resulting_enabled_features: LOGCAT_PIPE
}
hypervisor: KVM
)";

TEST(CommonReportedInfo, basic) {
    android_studio::EmulatorHost hostInfo;
    android_studio::EmulatorDetails details;
    EXPECT_TRUE(
        google::protobuf::TextFormat::ParseFromString(
            kEmulatorHostTestStr, &hostInfo));
    EXPECT_TRUE(
        google::protobuf::TextFormat::ParseFromString(
            kEmulatorDetailsTestStr, &details));

    CommonReportedInfo::setHostInfo(&hostInfo);
    CommonReportedInfo::setDetails(&details);

    // Test read/write from/to arrays (as std::string's)
    std::string tmpHostInfo;
    CommonReportedInfo::writeHostInfo(&tmpHostInfo);
    android_studio::EmulatorHost hostInfoFromCommon;
    EXPECT_TRUE(hostInfoFromCommon.ParseFromString(tmpHostInfo));
    EXPECT_EQ(hostInfo.DebugString(), hostInfoFromCommon.DebugString());

    std::string tmpDetails;
    CommonReportedInfo::writeDetails(&tmpDetails);
    android_studio::EmulatorDetails detailsFromCommon;
    EXPECT_TRUE(detailsFromCommon.ParseFromString(tmpDetails));
    EXPECT_EQ(details.DebugString(), detailsFromCommon.DebugString());

    // Test read/write from/to files.
    TempFile* tf = tempfile_create();
    tempfile_close(tf);
    {
        std::ofstream out(tempfile_path(tf), std::ios::out | std::ios::binary);
        CommonReportedInfo::writeHostInfo(&out);
    }
    {
        std::ifstream in(tempfile_path(tf), std::ios::in | std::ios::binary);
        CommonReportedInfo::readHostInfo(&in);

        std::string fromFileTmp;
        CommonReportedInfo::writeHostInfo(&fromFileTmp);
        android_studio::EmulatorHost hostInfoFromFile;
        EXPECT_TRUE(hostInfoFromFile.ParseFromString(fromFileTmp));
        EXPECT_EQ(hostInfo.DebugString(), hostInfoFromFile.DebugString());

    }

    {
        std::ofstream out(tempfile_path(tf), std::ios::out | std::ios::binary);
        CommonReportedInfo::writeDetails(&out);
    }
    {
        std::ifstream in(tempfile_path(tf), std::ios::in | std::ios::binary);
        CommonReportedInfo::readDetails(&in);

        std::string fromFileTmp;
        CommonReportedInfo::writeDetails(&fromFileTmp);
        android_studio::EmulatorDetails detailsFromFile;
        EXPECT_TRUE(detailsFromFile.ParseFromString(fromFileTmp));
        EXPECT_EQ(details.DebugString(), detailsFromFile.DebugString());

    }
}

} // android
