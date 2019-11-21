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

#include "metrics.h"

#include "android/base/async/ThreadLooper.h"
#include "android/base/files/IniFile.h"
#include "android/base/files/PathUtils.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/base/Optional.h"
#include "android/base/StringFormat.h"
#include "android/base/StringView.h"
#include "android/base/system/System.h"
#include "android/base/Uuid.h"
#include "android/CommonReportedInfo.h"
#include "android/cmdline-option.h"
#include "android/emulation/CpuAccelerator.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/globals.h"
#include "android/opengl/emugl_config.h"
#include "android/opengl/gpuinfo.h"
#include "android/opengles.h"
#include "android/metrics/AdbLivenessChecker.h"
#include "android/metrics/MetricsReporter.h"
#include "android/metrics/MemoryUsageReporter.h"
#include "android/metrics/PeriodicReporter.h"
#include "android/metrics/StudioConfig.h"
#include "android/utils/debug.h"
#include "android/utils/file_io.h"
#include "android/utils/x86_cpuid.h"

#include "android/metrics/proto/studio_stats.pb.h"

#include <utility>

using android::base::Optional;
using android::base::PathUtils;
using android::base::ScopedCPtr;
using android::base::StringView;
using android::base::System;
using android::metrics::MetricsReporter;
using android::metrics::PeriodicReporter;

namespace {

// A struct to contain all metrics reporters we need to be running for the
// whole emulator lifecycle.
// For now it's a single member, but soon to be more.
struct InstanceData {
    android::metrics::AdbLivenessChecker::Ptr livenessChecker;
    android::metrics::MemoryUsageReporter::Ptr memoryReporter;
    std::string emulatorName;

    void reset() {
        if (livenessChecker) {
            livenessChecker->stop();
            livenessChecker.reset();
        }
        if (memoryReporter) {
          memoryReporter->stop();
          memoryReporter.reset();
        }
    }
};

static android::base::LazyInstance<InstanceData> sGlobalData = {};

}  // namespace

bool android_metrics_start(const char* emulatorVersion,
                           const char* emulatorFullVersion,
                           const char* qemuVersion,
                           int controlConsolePort) {
    MetricsReporter::start(android::base::Uuid::generate().toString(),
                           emulatorVersion, emulatorFullVersion, qemuVersion);
    PeriodicReporter::start(&MetricsReporter::get(),
                            android::base::ThreadLooper::get());

    sGlobalData->emulatorName = android::base::StringFormat("emulator-%d",
                                    controlConsolePort);

    // Add a task that reports emulator's uptime (just in case that we don't
    // have enough of other messages reported).
    PeriodicReporter::get().addTask(
            5 * 60 * 1000,  // reporting period
            [](android_studio::AndroidStudioEvent* event) {
                // uptime fields are always filled for all events, so there's
                // nothing to do here.
                return true;
            });

    // For now only report memory usage on console when we are logging verbosely
    if (android_verbose) {
      sGlobalData->memoryReporter = android::metrics::MemoryUsageReporter::create(
          android::base::ThreadLooper::get(), 1000);
      sGlobalData->memoryReporter->start();
    }
    return true;
}

void android_metrics_stop(MetricsStopReason reason) {
    sGlobalData->reset();
    PeriodicReporter::stop();
    MetricsReporter::stop(reason);
}

// Start the ADB liveness monitor. call this when GUI starts
bool android_metrics_start_adb_liveness_checker(void *adbInterface)
{
    if (MetricsReporter::get().isReportingEnabled()) {
        android::emulation::AdbInterface *adb = (android::emulation::AdbInterface *)
                                                    adbInterface;
        std::string emulatorName = adb->serialString();
        if (emulatorName.empty())
            emulatorName = sGlobalData->emulatorName;
        sGlobalData->livenessChecker = android::metrics::AdbLivenessChecker::create(
                adb, android::base::ThreadLooper::get(), &MetricsReporter::get(),
                emulatorName, 20 * 1000);
        sGlobalData->livenessChecker->start();

        return true;
    } else {
        return false;
    }
}

static android_studio::EmulatorDetails::GuestCpuArchitecture
toClearcutLogGuestArch(const char* hw_cpu_arch) {
    using android_studio::EmulatorDetails;

    static constexpr std::pair<StringView,
                               EmulatorDetails::GuestCpuArchitecture>
            map[] = {
                    {"x86", EmulatorDetails::X86},
                    {"x86_64", EmulatorDetails::X86_64},
                    {"arm", EmulatorDetails::ARM},
                    {"arm64", EmulatorDetails::ARM_64},
                    {"mips", EmulatorDetails::MIPS},
                    {"mips64", EmulatorDetails::MIPS_64},
            };

    for (const auto& pair : map) {
        if (pair.first == hw_cpu_arch) {
            return pair.second;
        }
    }
    return EmulatorDetails::UNKNOWN_GUEST_CPU_ARCHITECTURE;
}

static android_studio::ProductDetails::CpuArchitecture
toClearcutLogCpuArchitecture(int bitness) {
    using android_studio::ProductDetails;
    switch (bitness) {
        case 32:
            return ProductDetails::X86;
        case 64:
            return ProductDetails::X86_64;
        default:
            return ProductDetails::UNKNOWN_CPU_ARCHITECTURE;
    }
}

static android_studio::ProductDetails::SoftwareLifeCycleChannel
toClearcutLogUpdateChannel(android::studio::UpdateChannel channel) {
    using android::studio::UpdateChannel;
    using android_studio::ProductDetails;
    switch (channel) {
        case UpdateChannel::Stable:
            return ProductDetails::STABLE;
        case UpdateChannel::Beta:
            return ProductDetails::BETA;
        case UpdateChannel::Dev:
            return ProductDetails::DEV;
        case UpdateChannel::Canary:
            return ProductDetails::CANARY;
        default:
            return ProductDetails::UNKNOWN_LIFE_CYCLE_CHANNEL;
    }
}

static android_studio::EmulatorDetails::EmulatorRenderer
toClearcutLogEmulatorRenderer(SelectedRenderer renderer) {
    // As of now, the enum values are exactly the same. Watch out for changes!
    return static_cast<android_studio::EmulatorDetails::EmulatorRenderer>(
            static_cast<int>(renderer));
}

static void fillGuestGlMetrics(android_studio::AndroidStudioEvent* event) {
    char* glVendor = nullptr;
    char* glRenderer = nullptr;
    char* glVersion = nullptr;
    // This call is only sensible after android_startOpenglesRenderer()
    // has been called.
    android_getOpenglesHardwareStrings(&glVendor, &glRenderer, &glVersion);
    if (glVendor) {
        event->mutable_emulator_details()->mutable_guest_gl()->set_vendor(
                glVendor);
        free(glVendor);
    }
    if (glRenderer) {
        event->mutable_emulator_details()->mutable_guest_gl()->set_renderer(
                glRenderer);
        free(glRenderer);
    }
    if (glVersion) {
        event->mutable_emulator_details()->mutable_guest_gl()->set_version(
                glVersion);
        free(glVersion);
    }
}

static Optional<int64_t> getAvdCreationTimeSec(AvdInfo* avd) {
    if (const auto createTime =
                System::get()->pathCreationTime(avdInfo_getContentPath(avd))) {
        return *createTime / 1000000;
    }
    // We were either unable to get the creation time, or this is Linux/really
    // old Mac and the system libs don't support creation times at all. Let's
    // use modification time for some rarely updated file in the avd,
    // e.g. <content_path>/data directory, or the root .ini.
    struct stat st;
    if (!android_stat(
                PathUtils::join(avdInfo_getContentPath(avd), "data").c_str(),
                &st)) {
        return st.st_mtime;
    }
    if (!android_stat(avdInfo_getRootIniPath(avd), &st)) {
        return st.st_mtime;
    }
    return {};
}

static void fillAvdFileInfo(
        android_studio::EmulatorAvdInfo* avdInfo,
        android_studio::EmulatorAvdFile::EmulatorAvdFileKind kind,
        const char* path,
        bool isCustom) {
    const auto file = avdInfo->add_files();
    file->set_kind(kind);
    System::FileSize size;
    if (System::get()->pathFileSize(path, &size)) {
        file->set_size(size);
    }
    if (auto creationTime = System::get()->pathCreationTime(path)) {
        file->set_creation_timestamp(*creationTime / 1000000);
    }
    file->set_location(isCustom ? android_studio::EmulatorAvdFile::CUSTOM
                                : android_studio::EmulatorAvdFile::STANDARD);
}

static android_studio::EmulatorAvdInfo::EmulatorAvdProperty
toClearcutLogAvdProperty(AvdFlavor flavor) {
    switch (flavor) {
    case AVD_PHONE:
        return android_studio::EmulatorAvdInfo::PHONE_AVD;
    case AVD_TV:
        return android_studio::EmulatorAvdInfo::TV_AVD;
    case AVD_WEAR:
        return android_studio::EmulatorAvdInfo::WEAR_AVD;
    case AVD_ANDROID_AUTO:
        return android_studio::EmulatorAvdInfo::ANDROIDAUTO_AVD;
    case AVD_OTHER:
        return android_studio::EmulatorAvdInfo::UNKNOWN_EMULATOR_AVD_FLAG;
    }
    return android_studio::EmulatorAvdInfo::UNKNOWN_EMULATOR_AVD_FLAG;
}

static void fillAvdMetrics(android_studio::AndroidStudioEvent* event) {
    VERBOSE_PRINT(metrics, "Filling AVD metrics");

    auto eventAvdInfo = event->mutable_emulator_details()->mutable_avd_info();
    // AVD name is a user-generated data, so won't report it.
    eventAvdInfo->set_api_level(avdInfo_getApiLevel(android_avdInfo));
    eventAvdInfo->set_image_kind(
            avdInfo_isGoogleApis(android_avdInfo)
                    ? android_studio::EmulatorAvdInfo::GOOGLE
                    : android_studio::EmulatorAvdInfo::AOSP);
    eventAvdInfo->set_arch(toClearcutLogGuestArch(android_hw->hw_cpu_arch));
    if (avdInfo_inAndroidBuild(android_avdInfo)) {
        // no real AVD, so no creation times or file infos.
        return;
    }

    if (auto creationTime = getAvdCreationTimeSec(android_avdInfo)) {
        eventAvdInfo->set_creation_timestamp(*creationTime);
        VERBOSE_PRINT(metrics, "AVD creation timestamp %ld", *creationTime);
    }

    const auto buildProps = avdInfo_getBuildProperties(android_avdInfo);
    android::base::IniFile ini((const char*)buildProps->data, buildProps->size);
    if (const int64_t buildTimestamp = ini.getInt64("ro.build.date.utc", 0)) {
        eventAvdInfo->set_build_timestamp(buildTimestamp);
        VERBOSE_PRINT(metrics, "AVD build timestamp %ld", buildTimestamp);
    }
    eventAvdInfo->set_build_id(ini.getString("ro.build.display.id", ""));

    fillAvdFileInfo(eventAvdInfo, android_studio::EmulatorAvdFile::KERNEL,
                android_hw->kernel_path,
                android_cmdLineOptions->kernel != nullptr);
    fillAvdFileInfo(
            eventAvdInfo, android_studio::EmulatorAvdFile::SYSTEM,
            (android_hw->disk_systemPartition_path && android_hw->disk_systemPartition_path[0])
                    ? android_hw->disk_systemPartition_path
                    : android_hw->disk_systemPartition_initPath,
            (android_cmdLineOptions->system || android_cmdLineOptions->sysdir));
    fillAvdFileInfo(eventAvdInfo, android_studio::EmulatorAvdFile::RAMDISK,
                android_hw->disk_ramdisk_path,
                android_cmdLineOptions->ramdisk != nullptr);

    eventAvdInfo->add_properties(
            toClearcutLogAvdProperty(avdInfo_getAvdFlavor(android_avdInfo)));
}

// Update this (and the proto) whenever feature flags change.
static android_studio::EmulatorFeatureFlagState::EmulatorFeatureFlag toClearcutFeatureFlag(
        android::featurecontrol::Feature feature) {
    switch (feature) {
        case android::featurecontrol::GLPipeChecksum:
            return android_studio::EmulatorFeatureFlagState::GL_PIPE_CHECKSUM;
        case android::featurecontrol::GrallocSync:
            return android_studio::EmulatorFeatureFlagState::GRALLOC_SYNC;
        case android::featurecontrol::EncryptUserData:
            return android_studio::EmulatorFeatureFlagState::ENCRYPT_USER_DATA;
        case android::featurecontrol::IntelPerformanceMonitoringUnit:
            return android_studio::EmulatorFeatureFlagState::INTEL_PERFORMANCE_MONITORING_UNIT;
        case android::featurecontrol::GLAsyncSwap:
            return android_studio::EmulatorFeatureFlagState::GL_ASYNC_SWAP;
        case android::featurecontrol::GLDMA:
            return android_studio::EmulatorFeatureFlagState::GLDMA;
        case android::featurecontrol::GLESDynamicVersion:
            return android_studio::EmulatorFeatureFlagState::GLES_DYNAMIC_VERSION;
        case android::featurecontrol::ForceANGLE:
            return android_studio::EmulatorFeatureFlagState::FORCE_ANGLE;
        case android::featurecontrol::ForceSwiftshader:
            return android_studio::EmulatorFeatureFlagState::FORCE_SWIFTSHADER;
        case android::featurecontrol::Wifi:
            return android_studio::EmulatorFeatureFlagState::WIFI;
        case android::featurecontrol::PlayStoreImage:
            return android_studio::EmulatorFeatureFlagState::PLAY_STORE_IMAGE;
        case android::featurecontrol::LogcatPipe:
            return android_studio::EmulatorFeatureFlagState::LOGCAT_PIPE;
        case android::featurecontrol::HYPERV:
            return android_studio::EmulatorFeatureFlagState::HYPERV;
        case android::featurecontrol::HVF:
            return android_studio::EmulatorFeatureFlagState::HVF;
        case android::featurecontrol::KVM:
            return android_studio::EmulatorFeatureFlagState::KVM;
        case android::featurecontrol::HAXM:
            return android_studio::EmulatorFeatureFlagState::HAXM;
        case android::featurecontrol::Feature_n_items:
            return android_studio::EmulatorFeatureFlagState::EMULATOR_FEATURE_FLAG_UNSPECIFIED;
    }
    return android_studio::EmulatorFeatureFlagState::EMULATOR_FEATURE_FLAG_UNSPECIFIED;
}

static void fillFeatureFlagState(android_studio::AndroidStudioEvent* event) {
    android_studio::EmulatorFeatureFlagState* featureFlagState =
        event->mutable_emulator_details()->mutable_feature_flag_state();

    for (const auto elt : android::featurecontrol::getEnabledNonOverride()) {
        featureFlagState->add_attempted_enabled_feature_flags(toClearcutFeatureFlag(elt));
    }

    for (const auto elt : android::featurecontrol::getEnabledOverride()) {
        featureFlagState->add_user_overridden_enabled_features(toClearcutFeatureFlag(elt));
    }

    for (const auto elt : android::featurecontrol::getDisabledOverride()) {
        featureFlagState->add_user_overridden_disabled_features(toClearcutFeatureFlag(elt));
    }

    for (const auto elt : android::featurecontrol::getEnabled()) {
        featureFlagState->add_resulting_enabled_features(toClearcutFeatureFlag(elt));
    }
}

void android_metrics_report_common_info(bool openglAlive) {
    MetricsReporter::get().report([openglAlive](
            android_studio::AndroidStudioEvent* event) {
        event->mutable_product_details()->set_channel(
                toClearcutLogUpdateChannel(android::studio::updateChannel()));
        event->mutable_product_details()->set_os_architecture(
                toClearcutLogCpuArchitecture(System::get()->getHostBitness()));

        event->mutable_emulator_details()->set_session_phase(
                android_studio::EmulatorDetails::RUNNING_GENERAL);
        event->mutable_emulator_details()->set_is_opengl_alive(openglAlive);
        event->mutable_emulator_details()->set_guest_arch(
                toClearcutLogGuestArch(android_hw->hw_cpu_arch));
        event->mutable_emulator_details()->set_guest_api_level(
                avdInfo_getApiLevel(android_avdInfo));

        // TODO: Check that CpuAccelerator enum +1 is the same
        // as the proto enum.
        event->mutable_emulator_details()->set_hypervisor(
                (android_studio::EmulatorDetails::EmulatorHypervisor)
                (android::GetCurrentCpuAccelerator() + 1));

        fillFeatureFlagState(event);

        event->mutable_emulator_details()->set_renderer(
                toClearcutLogEmulatorRenderer(
                        emuglConfig_get_renderer(android_hw->hw_gpu_mode)));

        event->mutable_emulator_details()->set_guest_gpu_enabled(
                android_hw->hw_gpu_enabled);

        if (android_hw->hw_gpu_enabled) {
            fillGuestGlMetrics(event);
        }

        for (const GpuInfo& gpu : globalGpuInfoList().infos) {
            auto hostGpu = event->mutable_emulator_details()->add_host_gpu();
            hostGpu->set_device_id(gpu.device_id);
            hostGpu->set_make(gpu.make);
            hostGpu->set_model(gpu.model);
            hostGpu->set_renderer(gpu.renderer);
            hostGpu->set_revision_id(gpu.revision_id);
            hostGpu->set_version(gpu.version);
        }

        fillAvdMetrics(event);

        event->mutable_emulator_host()->set_os_bit_count(
                    System::get()->getHostBitness());
        const AndroidCpuInfoFlags cpuFlags = android::GetCpuInfo().first;
        event->mutable_emulator_host()->set_virt_support(
                    cpuFlags & ANDROID_CPU_INFO_VIRT_SUPPORTED);
        event->mutable_emulator_host()->set_running_in_vm(
                    cpuFlags & ANDROID_CPU_INFO_VM);
        event->mutable_emulator_host()->set_cpu_manufacturer(
                    (cpuFlags & ANDROID_CPU_INFO_INTEL) ? "INTEL" :
                    (cpuFlags & ANDROID_CPU_INFO_AMD) ? "AMD" : "OTHER");

        uint32_t cpuid_mfs;
        android_get_x86_cpuid(1, 0, &cpuid_mfs,
                              nullptr, nullptr, nullptr);
        uint32_t cpuid_stepping  = cpuid_mfs & 0x0000000f;
        uint32_t cpuid_model     = (cpuid_mfs & 0x000000f0) >> 4;
        uint32_t cpuid_family    = (cpuid_mfs & 0x00000f00) >> 8;
        uint32_t cpuid_type      = (cpuid_mfs & 0x00003000) >> 12;
        uint32_t cpuid_extmodel  = (cpuid_mfs & 0x000f0000) >> 16;
        uint32_t cpuid_extfamily = (cpuid_mfs & 0x0ff00000) >> 20;
        event->mutable_emulator_host()->set_cpuid_stepping(cpuid_stepping);
        event->mutable_emulator_host()->set_cpuid_model(cpuid_model);
        event->mutable_emulator_host()->set_cpuid_family(cpuid_family);
        event->mutable_emulator_host()->set_cpuid_type(cpuid_type);
        event->mutable_emulator_host()->set_cpuid_extmodel(cpuid_extmodel);
        event->mutable_emulator_host()->set_cpuid_extfamily(cpuid_extfamily);

        {
            android_studio::EmulatorHost forCommonInfoHost =
                event->emulator_host();
            android_studio::EmulatorDetails forCommonInfoDetails =
                event->emulator_details();
            android::CommonReportedInfo::setHostInfo(&forCommonInfoHost);
            android::CommonReportedInfo::setDetails(&forCommonInfoDetails);
        }
    });
}
