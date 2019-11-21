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

#include "android/featurecontrol/HWMatching.h"
#include "android/featurecontrol/proto/emulator_feature_patterns.pb.h"
#include "google/protobuf/text_format.h"

#include <gtest/gtest.h>

namespace android {
namespace featurecontrol {

// Also test parsing protobuf comments.
static const char kTestFeaturePatterns[] = R"(
pattern { # 0
  hwconfig {
    hostinfo {
      cpu_manufacturer: "AMD"
      cpu_model_name: 0x0100f11 # Ryzen eng sample
    }
    hostgpuinfo { make: "10de" } # NVIDIA
  }
  featureaction { feature: GLESDynamicVersion enable: true }
}

pattern { # 1
  hwconfig {
    hostgpuinfo { make: "8086" } # Intel
  }
  featureaction { feature: GLAsyncSwap enable: false }
}

pattern { # 2
  hwconfig {
    hostinfo { os_platform: "Mac" }
  }
  featureaction { feature: GLESDynamicVersion enable: false }
}

pattern { # 3
  hwconfig {
    hostinfo { cpu_manufacturer: "KVMKVMKVM" }
  }
  featureaction { feature: HAXM enable: false }
}

pattern { # 4
  hwconfig {
    hostinfo { cpu_manufacturer: "KVMKVMKVM" }
    hostgpuinfo { make: "10de" }
  }
  featureaction { feature: MesaDRI enable: false }
}

pattern { # 5
  hwconfig {
    hostinfo { cpu_manufacturer: "KVMKVMKVM" }
    hostgpuinfo { make: "1002" }
  }
  featureaction { feature: AMDOpenGLDriver enable: true }
}

pattern { # 6
  hwconfig {
    hostinfo { os_platform: "Linux" }
  }
  featureaction { feature: MesaDRI enable: true }
  featureaction { feature: GLDMA } # test incomplete feature action
}

pattern { # 7
  hwconfig { hostinfo { os_platform: "Windows" } }
  hwconfig { hostinfo { os_platform: "Linux" } }
}

pattern { # 8
  hwconfig {}
}

pattern { # 9
}

)";

static const int kTestFeaturePatternsCount = 10;

TEST(HWMatching, parseFeaturePattern) {
    emulator_features::EmulatorFeaturePatterns patterns;
    EXPECT_TRUE(
            google::protobuf::TextFormat::ParseFromString(
                kTestFeaturePatterns, &patterns));
}

TEST(HWMatching, basicCpuMatch) {
    emulator_features::EmulatorFeaturePatterns patterns;
    EXPECT_TRUE(
            google::protobuf::TextFormat::ParseFromString(
                kTestFeaturePatterns, &patterns));
    EXPECT_TRUE(patterns.pattern_size() == kTestFeaturePatternsCount);

    HostHwInfo::Info testinfo = {
        "KVMKVMKVM",
        0, 1, 64, 0xf,
        "Linux",
        nullptr
    };

    EXPECT_FALSE(matchFeaturePattern(testinfo, &patterns.pattern(0)));
    EXPECT_FALSE(matchFeaturePattern(testinfo, &patterns.pattern(1)));
    EXPECT_FALSE(matchFeaturePattern(testinfo, &patterns.pattern(2)));
    EXPECT_TRUE(matchFeaturePattern(testinfo, &patterns.pattern(3)));
    EXPECT_FALSE(matchFeaturePattern(testinfo, &patterns.pattern(4)));
    EXPECT_FALSE(matchFeaturePattern(testinfo, &patterns.pattern(5)));
    EXPECT_TRUE(matchFeaturePattern(testinfo, &patterns.pattern(6)));
}

static GpuInfo makeTestGpuInfo(
        const std::string& make,
        const std::string& model,
        const std::string& device_id,
        const std::string& revision_id,
        const std::string& version,
        const std::string& renderer) {

    GpuInfo res;
    res.make = make;
    res.model = model;
    res.device_id = device_id;
    res.revision_id = revision_id;
    res.version = version;
    res.renderer = renderer;

    return res;
}

TEST(HWMatching, basicGpuMatch) {
    emulator_features::EmulatorFeaturePatterns patterns;
    EXPECT_TRUE(
            google::protobuf::TextFormat::ParseFromString(
                kTestFeaturePatterns, &patterns));
    EXPECT_TRUE(patterns.pattern_size() == kTestFeaturePatternsCount);


    GpuInfoList infolist;

    infolist.infos = {
        makeTestGpuInfo(
                "8086",
                "Intel Iris Pro",
                "", "", "", ""),
        makeTestGpuInfo(
                "10de",
                "NVIDIA GTX TITAN X",
                "", "", "", ""),
    };

    HostHwInfo::Info testinfo = {
        "KVMKVMKVM",
        0, 1, 64, 0xf,
        "Linux",
        &infolist,
    };

    EXPECT_FALSE(matchFeaturePattern(testinfo, &patterns.pattern(0)));
    EXPECT_TRUE(matchFeaturePattern(testinfo, &patterns.pattern(1)));
    EXPECT_FALSE(matchFeaturePattern(testinfo, &patterns.pattern(2)));
    EXPECT_TRUE(matchFeaturePattern(testinfo, &patterns.pattern(3)));
    EXPECT_TRUE(matchFeaturePattern(testinfo, &patterns.pattern(4)));
    EXPECT_FALSE(matchFeaturePattern(testinfo, &patterns.pattern(5)));
    EXPECT_TRUE(matchFeaturePattern(testinfo, &patterns.pattern(6)));
}

static bool actionsIdentical(
        const std::vector<FeatureAction>& a,
        const std::vector<FeatureAction>& b) {

    if (a.size() != b.size()) return false;

    for (size_t i = 0; i < a.size(); i++) {
        if (a[i].name != b[i].name) return false;
        if (a[i].enable != b[i].enable) return false;
    }

    return true;
}

TEST(HWMatching, basicFeatureActions) {
    emulator_features::EmulatorFeaturePatterns patterns;
    EXPECT_TRUE(
            google::protobuf::TextFormat::ParseFromString(
                kTestFeaturePatterns, &patterns));
    EXPECT_TRUE(patterns.pattern_size() == kTestFeaturePatternsCount);

    HostHwInfo::Info testinfo = {
        "KVMKVMKVM",
        0, 1, 64, 0xf,
        "Linux",
        nullptr
    };

    EXPECT_FALSE(matchFeaturePattern(testinfo, &patterns.pattern(0)));
    EXPECT_FALSE(matchFeaturePattern(testinfo, &patterns.pattern(1)));
    EXPECT_FALSE(matchFeaturePattern(testinfo, &patterns.pattern(2)));
    EXPECT_TRUE(matchFeaturePattern(testinfo, &patterns.pattern(3)));
    EXPECT_FALSE(matchFeaturePattern(testinfo, &patterns.pattern(4)));
    EXPECT_FALSE(matchFeaturePattern(testinfo, &patterns.pattern(5)));
    EXPECT_TRUE(matchFeaturePattern(testinfo, &patterns.pattern(6)));

    std::vector<FeatureAction> actions =
        matchFeaturePatterns(testinfo, &patterns);

    EXPECT_TRUE(actions.size() == 2);

    std::vector<FeatureAction> expectedActions = {
        { "HAXM", false },
        { "MesaDRI", true },
    };

    EXPECT_TRUE(actionsIdentical(actions, expectedActions));
}

TEST(HWMatching, basicDisjunction) {
    emulator_features::EmulatorFeaturePatterns patterns;
    EXPECT_TRUE(
            google::protobuf::TextFormat::ParseFromString(
                kTestFeaturePatterns, &patterns));
    EXPECT_TRUE(patterns.pattern_size() == kTestFeaturePatternsCount);

    HostHwInfo::Info testinfo = {
        "KVMKVMKVM",
        0, 1, 64, 0xf,
        "Linux",
        nullptr
    };

    EXPECT_TRUE(matchFeaturePattern(testinfo, &patterns.pattern(7)));
}

TEST(HWMatching, trivialMatch) {
    emulator_features::EmulatorFeaturePatterns patterns;
    EXPECT_TRUE(
            google::protobuf::TextFormat::ParseFromString(
                kTestFeaturePatterns, &patterns));
    EXPECT_TRUE(patterns.pattern_size() == kTestFeaturePatternsCount);

    HostHwInfo::Info testinfo = {
        "KVMKVMKVM",
        0, 1, 64, 0xf,
        "Linux",
        nullptr
    };

    EXPECT_TRUE(matchFeaturePattern(testinfo, &patterns.pattern(8)));
}

TEST(HWMatching, trivialMismatch) {
    emulator_features::EmulatorFeaturePatterns patterns;
    EXPECT_TRUE(
            google::protobuf::TextFormat::ParseFromString(
                kTestFeaturePatterns, &patterns));
    EXPECT_TRUE(patterns.pattern_size() == kTestFeaturePatternsCount);

    HostHwInfo::Info testinfo = {
        "KVMKVMKVM",
        0, 1, 64, 0xf,
        "Linux",
        nullptr
    };

    EXPECT_FALSE(matchFeaturePattern(testinfo, &patterns.pattern(9)));
}

} // featurecontrol
} // android
