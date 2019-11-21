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

#include "HWMatching.h"

#include "android/android-emu-version.h"
#include "android/base/containers/Lookup.h"
#include "android/base/files/PathUtils.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/StringView.h"
#include "android/base/system/System.h"
#include "android/base/threads/FunctorThread.h"
#include "android/curl-support.h"
#include "android/emulation/ConfigDirs.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/utils/filelock.h"

#include "android/featurecontrol/proto/emulator_feature_patterns.pb.h"
#include "google/protobuf/io/zero_copy_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/text_format.h"

#include <algorithm>
#include <fstream>

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
namespace featurecontrol {

static void queryFeaturePatternFn();

static const int kQueryFeaturePatternTimeoutMs = 5000;

class FeaturePatternQueryThread {
public:
    FeaturePatternQueryThread() :
        mThread([]() { queryFeaturePatternFn(); }) {
        mThread.start();
    }
private:
    android::base::FunctorThread mThread;
    DISALLOW_COPY_AND_ASSIGN(FeaturePatternQueryThread);
};

static LazyInstance<FeaturePatternQueryThread> sFeaturePatternQueryThread =
    LAZY_INSTANCE_INIT;

static const char kCachedPatternsFilename[] = "emu-last-feature-flags.protobuf";

static bool tryParseFeaturePatternsProtobuf(
        const std::string& filename,
        emulator_features::EmulatorFeaturePatterns* out_patterns) {
    std::ifstream in(filename);
    google::protobuf::io::IstreamInputStream istream(&in);

    if (!google::protobuf::TextFormat::Parse(
                &istream, out_patterns)) {
        D("failed to parse protobuf file");
        return false;
    }

    return true;
}

// Scoped accessor to help read the server-side feature patterns
// in an organized way, especially when other emulator instances
// are involved.
// Concurrent access may occur on |kCachedPatternsFilename| on
// multiple emulator instances.
// Current policy is to pretend the file is not there for the 2nd and later
// accessors of the file, if concurrent access happens.
class PatternsFileAccessor {
public:
    PatternsFileAccessor() :
        mFilename(
            android::base::PathUtils::join(
                android::ConfigDirs::getUserDirectory(),
                kCachedPatternsFilename)) { }

    ~PatternsFileAccessor() {
        if (mFileLock)
            filelock_release(mFileLock);
    }

    bool read(emulator_features::EmulatorFeaturePatterns* patterns) {
        if (!acquire()) return false;
        return tryParseFeaturePatternsProtobuf(mFilename, patterns);
    }

    bool write(emulator_features::EmulatorFeaturePatterns& patterns) {
        if (!acquire()) return false;

        {
            std::ofstream outFile(
                    mFilename,
                    std::ios_base::out | std::ios_base::trunc);

            if (!outFile) {
                D("not valid file: %s\n", mFilename.c_str());
                return false;
            }

            patterns.set_last_download_time(System::get()->getUnixTime());
            google::protobuf::io::OstreamOutputStream ostream(&outFile);
            google::protobuf::TextFormat::Print(patterns, &ostream);
        }

        // Check if we wrote a parseable protobuf, if not, delete immediately.
        emulator_features::EmulatorFeaturePatterns test_pattern;
        if (!tryParseFeaturePatternsProtobuf(mFilename, &test_pattern)) {
            D("we have invalid protobuf, delete it");
            System::get()->deleteFile(mFilename);
            return false;
        }

        return true;
    }
private:

    bool acquire() {
        if (mFileLock) {
            D("acquire() called twice by same process.");
            return false;
        }

        mFileLock = filelock_create(mFilename.c_str());

        if (!mFileLock) {
            D("another emulator process has lock or path is RO");
            return false;
        }

        D("successful acquire");
        return true;
    }

    std::string mFilename = {};
    FileLock* mFileLock = nullptr;

    DISALLOW_COPY_AND_ASSIGN(PatternsFileAccessor);
};

void asyncUpdateServerFeaturePatterns() {
    sFeaturePatternQueryThread.ptr();
}

bool matchFeaturePattern(
        const HostHwInfo::Info& hwinfo,
        const emulator_features::EmulatorFeaturePattern* pattern) {

    bool res = false;

    for (const auto& patternHwConfig : pattern->hwconfig()) {

        bool thisConfigMatches = true;

        if (patternHwConfig.has_hostinfo()) {

#define MATCH_FIELD(n) \
        if (patternHwConfig.hostinfo().has_##n()) \
            thisConfigMatches &= hwinfo.n == patternHwConfig.hostinfo().n(); \

            MATCH_FIELD(cpu_manufacturer)
            MATCH_FIELD(virt_support)
            MATCH_FIELD(running_in_vm)
            MATCH_FIELD(os_bit_count)
            MATCH_FIELD(cpu_model_name)
            MATCH_FIELD(os_platform)

        }

        // If the pattern contains GPU info, the system must
        // have GPU info that matches, or the pattern doesn't match.
        for (const auto& patternGpuInfo : patternHwConfig.hostgpuinfo()) {

            if (!hwinfo.gpuinfolist) {
                // If no gpu info list, fail match.
                thisConfigMatches = false;
                break;
            }

            bool matchesPatternGpu = false;

            for (const auto& hostgpuinfo : hwinfo.gpuinfolist->infos) {

                bool thisHostGpuMatches = true;

#define MATCH_GPU_FIELD(n) \
    if (patternGpuInfo.has_##n()) \
        thisHostGpuMatches &= hostgpuinfo.n == patternGpuInfo.n(); \

                MATCH_GPU_FIELD(make)
                MATCH_GPU_FIELD(model)
                MATCH_GPU_FIELD(device_id)
                MATCH_GPU_FIELD(revision_id)
                MATCH_GPU_FIELD(version)
                MATCH_GPU_FIELD(renderer)

                matchesPatternGpu |= thisHostGpuMatches;
            }

            thisConfigMatches &= matchesPatternGpu;
        }

        res |= thisConfigMatches;
    }

    return res;
}

static bool featureActionAppliesToEmulatorVersion(
    const emulator_features::FeatureFlagAction& action) {

    uint32_t minMajor, minMinor, minPatch;
    uint32_t maxMajor, maxMinor, maxPatch;

    bool appliesToCurrentVersion = true;

    // Allow version constraints in the protobuf
    // to be under-specified.
    if (action.has_min_version()) {
        if (action.min_version().has_major()) {
            minMajor = action.min_version().major();
            if (ANDROID_EMU_MAJOR_VERSION < minMajor) {
                appliesToCurrentVersion = false;
            } else if (ANDROID_EMU_MAJOR_VERSION == minMajor &&
                       action.min_version().has_minor()) {
                minMinor = action.min_version().minor();
                if (ANDROID_EMU_MINOR_VERSION < minMinor) {
                    appliesToCurrentVersion = false;
                } else if (ANDROID_EMU_MINOR_VERSION == minMinor &&
                           action.min_version().has_patch()) {
                    minPatch = action.min_version().patch();
                    if (ANDROID_EMU_PATCH_VERSION < minPatch)
                        appliesToCurrentVersion = false;
                }
            }
        }
    }

    if (action.has_max_version()) {
        if (action.max_version().has_major()) {
            maxMajor = action.max_version().major();
            if (ANDROID_EMU_MAJOR_VERSION > maxMajor) {
                appliesToCurrentVersion = false;
            } else if (ANDROID_EMU_MAJOR_VERSION == maxMajor &&
                       action.max_version().has_minor()) {
                maxMinor = action.max_version().minor();
                if (ANDROID_EMU_MINOR_VERSION > maxMinor) {
                    appliesToCurrentVersion = false;
                } else if (ANDROID_EMU_MINOR_VERSION == maxMinor &&
                           action.max_version().has_patch()) {
                    maxPatch = action.max_version().patch();
                    if (ANDROID_EMU_PATCH_VERSION > maxPatch)
                        appliesToCurrentVersion = false;
                }
            }
        }
    }

    return appliesToCurrentVersion;
}

std::vector<FeatureAction> matchFeaturePatterns(
        const HostHwInfo::Info& hwinfo,
        const emulator_features::EmulatorFeaturePatterns* input) {
    std::vector<FeatureAction> res;

    D("Num patterns: %d", input->pattern_size());

    for (const auto& pattern : input->pattern()) {

        bool thisHostMatches =
            matchFeaturePattern(hwinfo, &pattern);

        D("host match pattern? %d", thisHostMatches);

        if (!thisHostMatches) continue;

        for (const auto action : pattern.featureaction()) {

            if (!action.has_feature() || !action.has_enable()) continue;
            if (!featureActionAppliesToEmulatorVersion(action)) continue;

            FeatureAction nextAction;
            nextAction.name = emulator_features::Feature_Name(action.feature());
            nextAction.enable = action.enable();
            res.push_back(nextAction);
        }
    }

    return res;
}

static void doFeatureAction(const FeatureAction& action) {

    Feature feature = stringToFeature(action.name);

    if (stringToFeature(action.name) == Feature::Feature_n_items) return;

    // Conservatively:
    // If the server wants to enable: enable if guest didn't disable
    // If the server wants to disable: disable if user did not override
    if (action.enable) {
        setIfNotOverridenOrGuestDisabled(feature, action.enable);
        D("server has tried to enable %s", action.name.c_str());
    } else {
        setIfNotOverriden(feature, action.enable);
        D("server has tried to disable %s", action.name.c_str());
    }
}

static const char kFeaturePatternsUrlPrefix[] =
    "https://dl.google.com/dl/android/studio/metadata/emulator-feature-flags.protobuf";

static size_t curlDownloadFeaturePatternsCallback(
        char* contents, size_t size, size_t nmemb, void* userp) {

    size_t total = size * nmemb;

    auto& res = *static_cast<std::string*>(userp);
    res.append(contents, contents + total);

    return total;
}

std::string downloadFeaturePatterns() {
    D("load: %s\n", kFeaturePatternsUrlPrefix);

    char* curlError = nullptr;
    std::string res;
    if (!curl_download(
            kFeaturePatternsUrlPrefix, nullptr,
            &curlDownloadFeaturePatternsCallback,
            &res, &curlError)) {
        return "Error: failed to download feature patterns from server.";
    }

    D("got: %s", res.c_str());
    return res;
}

static void outputCachedFeaturePatterns(
        emulator_features::EmulatorFeaturePatterns& patterns) {
    PatternsFileAccessor access;

    access.write(patterns);
}

static LazyInstance<emulator_features::EmulatorFeaturePatterns> sCachedFeaturePatterns =
    LAZY_INSTANCE_INIT;

void applyCachedServerFeaturePatterns() {
    PatternsFileAccessor access;

    if (!access.read(sCachedFeaturePatterns.ptr()))
        return;

    std::vector<FeatureAction> todo =
        matchFeaturePatterns(HostHwInfo::query(), sCachedFeaturePatterns.ptr());

    for (const auto& elt : todo) {
        doFeatureAction(elt);
    }
}

static const uint32_t kFeaturePatternDownloadIntervalSeconds = 24 * 60 * 60;

static void queryFeaturePatternFn() {

    uint32_t currTime = System::get()->getUnixTime();

    if (currTime - sCachedFeaturePatterns->last_download_time() <
        kFeaturePatternDownloadIntervalSeconds) {
        D("Not downloading; last download time %u current time %u",
          sCachedFeaturePatterns->last_download_time(),
          currTime);
        return;
    } else {
        D("Downloading new feature patterns.");
    }

    std::string featurePattern =
        downloadFeaturePatterns();
    emulator_features::EmulatorFeaturePatterns patterns;
    if (!google::protobuf::TextFormat::ParseFromString(
                featurePattern, &patterns)) {
        D("Downloaded protobuf file corrupt.");
        return;
    }
    outputCachedFeaturePatterns(patterns);
}


} // namespace hwinfo
} // namespace android
