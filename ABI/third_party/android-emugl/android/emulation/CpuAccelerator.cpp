// Copyright (C) 2014 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#define CPU_ACCELERATOR_PRIVATE
#include "android/emulation/CpuAccelerator.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <winioctl.h>
#else
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <stdio.h>

#include "android/base/Compiler.h"
#include "android/base/files/ScopedFd.h"
#include "android/base/Log.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/base/misc/FileUtils.h"
#include "android/base/StringFormat.h"
#include "android/base/system/System.h"
#include "android/cpu_accelerator.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/utils/file_data.h"
#include "android/utils/file_io.h"
#include "android/utils/path.h"
#include "android/utils/tempfile.h"
#include "android/utils/x86_cpuid.h"

#ifdef _WIN32
#include "android/base/files/ScopedFileHandle.h"
#include "android/windows_installer.h"
#include "android/base/system/Win32UnicodeString.h"
#include "android/base/files/PathUtils.h"
#endif

#ifdef __APPLE__
#include "android/emulation/internal/CpuAccelerator.h"
#endif

#include <array>

// NOTE: This source file must be independent of the rest of QEMU, as such
//       it should not include / reuse any QEMU source file or function
//       related to KVM or HAX.

#ifdef __linux__
#  define  HAVE_KVM  1
#  define  HAVE_HAX  0
#elif defined(_WIN32) || defined(__APPLE__)
#  define HAVE_KVM 0
#  define HAVE_HAX 1

#if defined(__APPLE__)
#  define HAVE_HVF 1
#endif

#else
#  error "Unsupported host platform!"
#endif

namespace android {

using base::StringAppendFormat;
using base::ScopedFd;
using base::Version;

namespace {

struct GlobalState {
    bool probed;
    bool testing;
    CpuAccelerator accel;
    char status[256];
    char version[256];
    AndroidCpuAcceleration status_code;
    std::array<bool, CPU_ACCELERATOR_MAX> supported_accelerators;
};

GlobalState gGlobals = {false,  false,  CPU_ACCELERATOR_NONE,
                        {'\0'}, {'\0'}, ANDROID_CPU_ACCELERATION_ERROR,
                        {}};

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////
/////   Linux KVM support.
/////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

#if HAVE_KVM

#include <linux/kvm.h>

#include "android/emulation/kvm_env.h"

// Return true iff KVM is installed and usable on this machine.
// |*status| will be set to a small status string explaining the
// status of KVM on success or failure.
AndroidCpuAcceleration ProbeKVM(std::string* status) {
    const char* kvm_device = getenv(KVM_DEVICE_NAME_ENV);
    if (NULL == kvm_device) {
      kvm_device = "/dev/kvm";
    }
    // Check that kvm device exists.
    if (::access(kvm_device, F_OK)) {
        // kvm device does not exist
        bool cpu_ok =
                android_get_x86_cpuid_vmx_support() ||
                android_get_x86_cpuid_svm_support();
        if (!cpu_ok) {
            status->assign(
                "KVM requires a CPU that supports vmx or svm");
            return ANDROID_CPU_ACCELERATION_NO_CPU_SUPPORT;
        }
        StringAppendFormat(status,
                           "%s is not found: VT disabled in BIOS or KVM kernel "
                           "module not loaded", kvm_device);
        return ANDROID_CPU_ACCELERATION_DEV_NOT_FOUND;
    }

    // Check that kvm device can be opened.
    if (::access(kvm_device, R_OK)) {
        StringAppendFormat(status,
                           "This user doesn't have permissions to use KVM (%s)",
                           kvm_device);
        return ANDROID_CPU_ACCELERATION_DEV_PERMISSION;
    }

    // Open the file.
    ScopedFd fd(TEMP_FAILURE_RETRY(open(kvm_device, O_RDWR)));
    if (!fd.valid()) {
        StringAppendFormat(status, "Could not open %s : %s", kvm_device,
                           strerror(errno));
        return ANDROID_CPU_ACCELERATION_DEV_OPEN_FAILED;
    }

    // Extract KVM version number.
    int version = ::ioctl(fd.get(), KVM_GET_API_VERSION, 0);
    if (version < 0) {
        status->assign("Could not extract KVM version: ");
        status->append(strerror(errno));
        return ANDROID_CPU_ACCELERATION_DEV_IOCTL_FAILED;
    }

    // Compare to minimum supported version
    status->clear();

    if (version < KVM_API_VERSION) {
        StringAppendFormat(status,
                           "KVM version too old: %d (expected at least %d)\n",
                           version,
                           KVM_API_VERSION);
        return ANDROID_CPU_ACCELERATION_DEV_OBSOLETE;
    }

    // Profit!
    StringAppendFormat(status,
                       "KVM (version %d) is installed and usable.",
                       version);
    GlobalState* g = &gGlobals;
    ::snprintf(g->version, sizeof(g->version), "%d", version);
    return ANDROID_CPU_ACCELERATION_READY;
}

#endif  // HAVE_KVM


#if HAVE_HAX

#define HAXM_INSTALLER_VERSION_MINIMUM 0x06000001

std::string cpuAcceleratorFormatVersion(int32_t version) {
    if (version < 0) {
        return "<invalid>";
    }
    char buf[16];  // strlen("127.255.65535")+1 = 14
    int32_t revision = version & 0xffff;
    version >>= 16;
    int32_t minor = version & 0xff;
    version >>= 8;
    int32_t major = version & 0x7f;
    snprintf(buf, sizeof(buf), "%i.%i.%i", major, minor, revision);
    return buf;
}

#ifdef __APPLE__

}  // namespace

int32_t cpuAcceleratorParseVersionScript(const std::string& version_script) {
    int32_t result = 0;
    const char ver[] = "VERSION=";
    const size_t ver_len = sizeof(ver) - 1U;

    size_t offset = version_script.find(ver);
    if (offset == std::string::npos) {
        return -1;
    }
    const char* pos = version_script.c_str() + ver_len;

    const int kValueCountMax = 3;  // support 3 numbers max major.minor.rev
    const int bit_offset[kValueCountMax] = {24, 16, 0};
    const int value_max[kValueCountMax] = {127, 255, 65535};
    for (int i = 0; i < kValueCountMax; i++) {
        const char* end = pos;
        unsigned long value = strtoul(pos, (char**)&end, 10);
        if (pos == end) {
            // no number was found, error if there was not at least one.
            if (i == 0) {
                result = -1;
            }
            break;
        }

        if (value > value_max[i]) {
            // invalid number was found
            return -1;
        }

        result |= (value << bit_offset[i]);

        if (*end == '.') {
            // skip delimiter
            end += 1;
        } else {
            // we're done parsing
            break;
        }
        // advance to next number
        pos = end;
    }
    // 0 is an invalid version number.
    if (result == 0) {
        result = -1;
    }
    return result;
}

int32_t cpuAcceleratorGetHaxVersion(const char* kext_dir[],
                                    const size_t kext_dir_count,
                                    const char* version_file) {
    bool found_haxm_kext = false;

    for (size_t i = 0; i < kext_dir_count; i++) {
        struct stat s;
        int err = android_stat(kext_dir[i], &s);
        if (err < 0 || !S_ISDIR(s.st_mode)) {
            // dir not found
            continue;
        }
        // At this point, we're certain that haxm is installed,
        // but might be broken
        found_haxm_kext = true;

        std::string version_file_abs =
                std::string(kext_dir[i]) + "/" + version_file;
        FileData fd;
        if (fileData_initFromFile(&fd, version_file_abs.c_str()) < 0) {
            // let's try the next directory, just in case
            continue;
        }

        // File data contains a bash variable assignment
        // e.g. "VERSION=1.1.4"
        std::string version_script(fd.data, fd.data + fd.size);
        int32_t result = cpuAcceleratorParseVersionScript(version_script);
        if (result == 0) {
            // This function uses a return value of zero to indicate "not
            // installed"
            // So I'm declaring version "0.0.0" to be invalid
            result = -1;
        }
        if (result <= -1) {
            // let's try the next directory, just in case
            continue;
        }
        return result;
    }

    if (found_haxm_kext) {
        // found haxm kext but couldn't parse version file
        return -1;
    }
    // not installed
    return 0;
}

namespace {

#endif  // __APPLE__

// Version numbers for the HAX kernel module.
// |compat_version| is the minimum API version supported by the module.
// |current_version| is its current API version.
struct HaxModuleVersion {
    uint32_t compat_version;
    uint32_t current_version;
};

/*
 * ProbeHaxCpu: returns ANDROID_CPU_ACCELERATION_READY if the CPU supports
 * HAXM requirements.
 *
 * Otherwise returns some other AndroidCpuAcceleration status and sets
 * |status| to a user-understandable error string
 */
AndroidCpuAcceleration ProbeHaxCpu(std::string* status) {
    char vendor_id[16];
    android_get_x86_cpuid_vendor_id(vendor_id, sizeof(vendor_id));

    if (android_get_x86_cpuid_vendor_id_is_vmhost(vendor_id)) {

        StringAppendFormat(status,
                           "Android Emulator does not support nested virtualization.  "
                           "Your CPU: '%s'",
                           vendor_id);
        return ANDROID_CPU_ACCELERATION_NESTED_NOT_SUPPORTED;
    }

    /* HAXM only supports GenuineIntel processors */
    if (android_get_x86_cpuid_vendor_id_type(vendor_id) != VENDOR_ID_INTEL) {
        StringAppendFormat(status,
                           "Android Emulator requires an Intel processor with "
                           "VT-x and NX support.  Your CPU: '%s'",
                           vendor_id);
        return ANDROID_CPU_ACCELERATION_NO_CPU_SUPPORT;
    }

    if (!android_get_x86_cpuid_vmx_support()) {
        if (android_get_x86_cpuid_is_vcpu()) {
#ifdef _WIN32
            // The vcpu bit is set but your vendor id is not one of the known VM ids
            // You are probably running under Hyper-V
            status->assign("Please disable Hyper-V before using the Android Emulator.  "
                           "Start a command prompt as Administrator, run 'bcdedit /set "
                           "hypervisorlaunchtype off', reboot.");
            return ANDROID_CPU_ACCELERATION_HYPERV_ENABLED;
#else // OSX
            StringAppendFormat(status,
                               "Android Emulator does not support nested virtualization.  "
                               "Your CPU: '%s'",
                               vendor_id);
            return ANDROID_CPU_ACCELERATION_NESTED_NOT_SUPPORTED;
#endif
        }
        status->assign(
            "Android Emulator requires an Intel processor with VT-x and NX support.  "
            "(VT-x is not supported)");
        return ANDROID_CPU_ACCELERATION_NO_CPU_VTX_SUPPORT;
    }

    if (!android_get_x86_cpuid_nx_support()) {
        status->assign(
            "Android Emulator requires an Intel processor with VT-x and NX support.  "
            "(NX is not supported)");
        return ANDROID_CPU_ACCELERATION_NO_CPU_NX_SUPPORT;
    }

    return ANDROID_CPU_ACCELERATION_READY;
}

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////
/////  Windows HAX support.
/////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

#if defined(_WIN32)

using base::ScopedFileHandle;
using namespace android;

// Windows IOCTL code to extract HAX kernel module version.
#define HAX_DEVICE_TYPE 0x4000
#define HAX_IOCTL_VERSION       \
    CTL_CODE(HAX_DEVICE_TYPE, 0x900, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define HAX_IOCTL_CAPABILITY    \
    CTL_CODE(HAX_DEVICE_TYPE, 0x910, METHOD_BUFFERED, FILE_ANY_ACCESS)

// The minimum API version supported by the Android emulator.
#define HAX_MIN_VERSION  3 // 6.0.0

// IMPORTANT: Keep in sync with target-i386/hax-interface.h
struct hax_capabilityinfo
{
    /* bit 0: 1 - HAXM is working
     *        0 - HAXM is not working possibly because VT/NX is disabled
                  NX means Non-eXecution, aks. XD (eXecution Disable)
     * bit 1: 1 - HAXM has hard limit on how many RAM can be used as guest RAM
     *        0 - HAXM has no memory limitation
     */
#define HAX_CAP_STATUS_WORKING  0x1
#define HAX_CAP_STATUS_NOTWORKING  0x0
#define HAX_CAP_WORKSTATUS_MASK 0x1
#define HAX_CAP_MEMQUOTA        0x2
    uint16_t wstatus;
    /*
     * valid when HAXM is not working
     * bit 0: HAXM is not working because VT is not enabeld
     * bit 1: HAXM is not working because NX not enabled
     */
#define HAX_CAP_FAILREASON_VT   0x1
#define HAX_CAP_FAILREASON_NX   0x2
    uint16_t winfo;
    uint32_t pad;
    uint64_t mem_quota;
};

AndroidCpuAcceleration ProbeHAX(std::string* status) {
    status->clear();

    AndroidCpuAcceleration cpu = ProbeHaxCpu(status);
    if (cpu != ANDROID_CPU_ACCELERATION_READY)
        return cpu;

    const char* productDisplayName = u8"Intel® Hardware Accelerated Execution Manager";
    int32_t haxm_installer_version = WindowsInstaller::getVersion(productDisplayName);
    if (haxm_installer_version == 0) {
        status->assign(
            "HAXM is not installed on this machine");
        return ANDROID_CPU_ACCELERATION_ACCEL_NOT_INSTALLED;
    }

    if (haxm_installer_version < HAXM_INSTALLER_VERSION_MINIMUM) {
        StringAppendFormat(
                status, "HAXM must be updated (version %s < %s).",
                cpuAcceleratorFormatVersion(haxm_installer_version),
                cpuAcceleratorFormatVersion(HAXM_INSTALLER_VERSION_MINIMUM));
        return ANDROID_CPU_ACCELERATION_ACCEL_OBSOLETE;
    }

    // 1) Try to find the HAX kernel module.
    ScopedFileHandle hax(CreateFile("\\\\.\\HAX",
                                    GENERIC_READ | GENERIC_WRITE,
                                    0,
                                    NULL,
                                    CREATE_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL));
    if (!hax.valid()) {
        DWORD err = GetLastError();
        if (err == ERROR_FILE_NOT_FOUND) {
            status->assign(
                "Unable to open HAXM device: ERROR_FILE_NOT_FOUND");
            return ANDROID_CPU_ACCELERATION_DEV_NOT_FOUND;
        } else if (err == ERROR_ACCESS_DENIED) {
            status->assign(
                "Unable to open HAXM device: ERROR_ACCESS_DENIED");
            return ANDROID_CPU_ACCELERATION_DEV_PERMISSION;
        }
        StringAppendFormat(status,
                           "Opening HAX kernel module failed: %u",
                           err);
        return ANDROID_CPU_ACCELERATION_DEV_OPEN_FAILED;
    }

    // 2) Extract the module's version.
    HaxModuleVersion hax_version;

    DWORD dSize = 0;
    BOOL ret = DeviceIoControl(hax.get(),
                               HAX_IOCTL_VERSION,
                               NULL, 0,
                               &hax_version, sizeof(hax_version),
                               &dSize,
                               (LPOVERLAPPED) NULL);
    if (!ret) {
        DWORD err = GetLastError();
        StringAppendFormat(status,
                            "Could not extract HAX module version: %u",
                            err);
        return ANDROID_CPU_ACCELERATION_DEV_IOCTL_FAILED;
    }

    // 3) Check that it is the right version.
    if (hax_version.current_version < HAX_MIN_VERSION) {
        StringAppendFormat(status,
                           "HAX version (%d) is too old (need at least %d).",
                           hax_version.current_version,
                           HAX_MIN_VERSION);
        return ANDROID_CPU_ACCELERATION_DEV_OBSOLETE;
    }

    hax_capabilityinfo cap = {};
    ret = DeviceIoControl(hax.get(),
                          HAX_IOCTL_CAPABILITY,
                          NULL, 0,
                          &cap, sizeof(cap),
                          &dSize,
                          (LPOVERLAPPED) NULL);

    if (!ret) {
        DWORD err = GetLastError();
        StringAppendFormat(status,
                            "Could not extract HAX capability: %u",
                            err);
        return ANDROID_CPU_ACCELERATION_DEV_IOCTL_FAILED;
    }

    if ( (cap.wstatus & HAX_CAP_WORKSTATUS_MASK) == HAX_CAP_STATUS_NOTWORKING )
    {
        if (cap.winfo & HAX_CAP_FAILREASON_VT) {
            status->assign("VT feature disabled in BIOS/UEFI");
            return ANDROID_CPU_ACCELERATION_VT_DISABLED;
        }
        else if (cap.winfo & HAX_CAP_FAILREASON_NX) {
            status->assign("NX feature disabled in BIOS/UEFI");
            return ANDROID_CPU_ACCELERATION_NX_DISABLED;
        }
    }

    // 4) Profit!
    StringAppendFormat(
            status, "HAXM version %s (%d) is installed and usable.",
            cpuAcceleratorFormatVersion(haxm_installer_version),
            hax_version.current_version);
    GlobalState* g = &gGlobals;
    ::snprintf(g->version, sizeof(g->version), "%s",
               cpuAcceleratorFormatVersion(haxm_installer_version).c_str());
    return ANDROID_CPU_ACCELERATION_READY;
}

#elif defined(__APPLE__)

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////
/////  Darwin HAX support.
/////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

// An IOCTL command number used to retrieve the HAX kernel module version.
#define HAX_IOCTL_VERSION _IOWR(0, 0x20, HaxModuleVersion)

// The minimum API version supported by the Android emulator.
#define HAX_MIN_VERSION  3 // 6.0.0

AndroidCpuAcceleration ProbeHAX(std::string* status) {
    AndroidCpuAcceleration cpu = ProbeHaxCpu(status);
    if (cpu != ANDROID_CPU_ACCELERATION_READY)
        return cpu;

    const char* kext_dir[] = {
        "/Library/Extensions/intelhaxm.kext", // current
        "/System/Library/Extensions/intelhaxm.kext", // old
    };
    size_t kext_dir_count = sizeof(kext_dir)/sizeof(kext_dir[0]);

    const char* version_file = "Contents/Resources/support.txt";
    int32_t version = cpuAcceleratorGetHaxVersion(
        kext_dir,
        kext_dir_count,
        version_file
        );
    if (version == 0) {
        status->assign(
            "HAXM is not installed on this machine");
        return ANDROID_CPU_ACCELERATION_ACCEL_NOT_INSTALLED;
    }

    if (version < HAXM_INSTALLER_VERSION_MINIMUM) {
        // HAXM was found but version number was too old or missing
        StringAppendFormat(
                status, "HAXM must be updated (version %s < %s).",
                cpuAcceleratorFormatVersion(version),
                cpuAcceleratorFormatVersion(HAXM_INSTALLER_VERSION_MINIMUM));
        return ANDROID_CPU_ACCELERATION_ACCEL_OBSOLETE;
    }

    // 1) Check that /dev/HAX exists.
    if (::access("/dev/HAX", F_OK)) {
        status->assign(
            "HAXM is not installed on this machine (/dev/HAX is missing).");
        return ANDROID_CPU_ACCELERATION_DEV_NOT_FOUND;
    }

    // 2) Check that /dev/HAX can be opened.
    if (::access("/dev/HAX", R_OK)) {
        status->assign(
            "This user doesn't have permission to use HAX (/dev/HAX).");
        return ANDROID_CPU_ACCELERATION_DEV_PERMISSION;
    }

    // 3) Open the file.
    ScopedFd fd(open("/dev/HAX", O_RDWR));
    if (!fd.valid()) {
        status->assign("Could not open /dev/HAX: ");
        status->append(strerror(errno));
        return ANDROID_CPU_ACCELERATION_DEV_OPEN_FAILED;
    }

    // 4) Extract HAX version number.
    status->clear();

    HaxModuleVersion hax_version;
    if (::ioctl(fd.get(), HAX_IOCTL_VERSION, &hax_version) < 0) {
        StringAppendFormat(status,
                           "Could not extract HAX version: %s",
                           strerror(errno));
        return ANDROID_CPU_ACCELERATION_DEV_IOCTL_FAILED;
    }

    if (hax_version.current_version < hax_version.compat_version) {
        StringAppendFormat(
                status,
                "Malformed HAX version numbers (current=%d, compat=%d)\n",
                hax_version.current_version,
                hax_version.compat_version);
        return ANDROID_CPU_ACCELERATION_DEV_OBSOLETE;
    }

    // 5) Compare to minimum supported version.

    if (hax_version.current_version < HAX_MIN_VERSION) {
        StringAppendFormat(status,
                           "HAX API version too old: %d (expected at least %d)\n",
                           hax_version.current_version,
                           HAX_MIN_VERSION);
        return ANDROID_CPU_ACCELERATION_DEV_OBSOLETE;
    }

    // 6) Profit!
    StringAppendFormat(status, "HAXM version %s (%d) is installed and usable.",
                       cpuAcceleratorFormatVersion(version),
                       hax_version.current_version);
    GlobalState* g = &gGlobals;
    ::snprintf(g->version, sizeof(g->version), "%s",
               cpuAcceleratorFormatVersion(version).c_str());
    return ANDROID_CPU_ACCELERATION_READY;
}


/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////
/////  Darwin Hypervisor.framework support.
/////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

#if HAVE_HVF

using android::base::System;

// Technically, we need  rdmsr to detect ept/ug support,
// but that instruction is only available
// as root. So just say yes if the processor has features that were
// only introduced the same time as the feature.
// EPT: popcnt
// UG: aes
static bool cpuCanRunHVF() {
    uint32_t cpuid_function1_ecx;
    android_get_x86_cpuid(1, 0, NULL, NULL, &cpuid_function1_ecx, NULL);

    uint32_t popcnt_support = 1 << 23;
    uint32_t aes_support = 1 << 25;

    bool eptSupport = cpuid_function1_ecx & popcnt_support;
    bool ugSupport = cpuid_function1_ecx & aes_support;

    return eptSupport && ugSupport;
}

AndroidCpuAcceleration ProbeHVF(std::string* status) {
    status->clear();

    AndroidCpuAcceleration res = ANDROID_CPU_ACCELERATION_NO_CPU_SUPPORT;
    std::string osProductVersion = System::get()->getOsName();
    // Example os version :  Max OS X 10.12.4
    size_t pos = osProductVersion.rfind(" ");
    if (strncmp("Error: ", osProductVersion.c_str(), 7) == 0 ||
        pos == std::string::npos) {
      StringAppendFormat(status,
                         "Internal error: failed to parse OS version '%s'",
                         osProductVersion);
      return res;
    }

    auto ver = Version(
            osProductVersion.substr(pos + 1, osProductVersion.size() - pos));
    if (!ver.isValid()) {
        StringAppendFormat(status,
                           "Internal error: failed to parse OS version '%s'",
                           osProductVersion);
        return res;
    }

    // Need same virtualization features as required by HAXM.
    if (ProbeHaxCpu(status) != ANDROID_CPU_ACCELERATION_READY) {
        return res;
    }

    // Also need EPT and UG
    if (!cpuCanRunHVF()) {
        status->assign(
                "CPU doesn't support EPT and/or UG features "
                "needed for Hypervisor.Framework");
        return res;
    }

    // Hypervisor.framework is only supported on OS X 10.10 and above.
    if (ver < Version(10, 10, 0)) {
        StringAppendFormat(status,
                           "Hypervisor.Framework is only supported"
                           "on OS X 10.10 and above");
        return res;
    } else {
        res = ANDROID_CPU_ACCELERATION_READY;
    }

    GlobalState* g = &gGlobals;
    int maj = ver.component<Version::kMajor>();
    int min = ver.component<Version::kMinor>();
    ::snprintf(g->version, sizeof(g->version), "%d.%d", maj, min);
    StringAppendFormat(status, "Hypervisor.Framework OS X Version %d.%d", maj,
                       min);
    return res;
}
#endif // HAVE_HVF

#else   // !_WIN32 && !__APPLE__
#error "Unsupported HAX host platform"
#endif  // !_WIN32 && !__APPLE__

#endif  // HAVE_HAX

}  // namespace

CpuAccelerator GetCurrentCpuAccelerator() {
    GlobalState* g = &gGlobals;

    if (g->probed || g->testing) {
        return g->accel;
    }

    g->supported_accelerators = {};

    std::string status;
#if HAVE_KVM
    AndroidCpuAcceleration status_code = ProbeKVM(&status);
    if (status_code == ANDROID_CPU_ACCELERATION_READY) {
        g->accel = CPU_ACCELERATOR_KVM;
        g->supported_accelerators[CPU_ACCELERATOR_KVM] = true;
    }
#elif HAVE_HAX || HAVE_HVF
    AndroidCpuAcceleration status_code = ProbeHAX(&status);
    if (status_code == ANDROID_CPU_ACCELERATION_READY) {
        g->accel = CPU_ACCELERATOR_HAX;
        g->supported_accelerators[CPU_ACCELERATOR_HAX] = true;
    }
#if HAVE_HVF
    if (featurecontrol::isEnabled(featurecontrol::HVF)) {
        std::string statusHvf;
        AndroidCpuAcceleration status_code_HVF = ProbeHVF(&statusHvf);
        if (status_code_HVF == ANDROID_CPU_ACCELERATION_READY) {
            g->supported_accelerators[CPU_ACCELERATOR_HVF] = true;
            // TODO: Switch to HVF as default option if/when appropriate.
            if (status_code != ANDROID_CPU_ACCELERATION_READY) {
                g->accel = CPU_ACCELERATOR_HVF;
                status_code = status_code_HVF;
                status = std::move(statusHvf);
            }
        }
    }
#endif  // HAVE_HFV

#else  // !HAVE_KVM && !(HAVE_HAX || HAVE_HVF)
    status = "This system does not support CPU acceleration.";
#endif  // !HAVE_KVM && !(HAVE_HAX || HAVE_HVF)

    // cache status
    g->probed = true;
    g->status_code = status_code;
    ::snprintf(g->status, sizeof(g->status), "%s", status.c_str());

    return g->accel;
}

void ResetCurrentCpuAccelerator(CpuAccelerator accel) {
    GlobalState *g = &gGlobals;
    g->accel = accel;
}

bool GetCurrentAcceleratorSupport(CpuAccelerator type) {
    GlobalState *g = &gGlobals;

    if (!g->probed && !g->testing) {
        // Force detection of the current CPU accelerator.
        GetCurrentCpuAccelerator();
    }

    return g->supported_accelerators[type];
}

std::string GetCurrentCpuAcceleratorStatus() {
    GlobalState *g = &gGlobals;

    if (!g->probed && !g->testing) {
        // Force detection of the current CPU accelerator.
        GetCurrentCpuAccelerator();
    }

    return std::string(g->status);
}

AndroidCpuAcceleration GetCurrentCpuAcceleratorStatusCode() {
    GlobalState *g = &gGlobals;

    if (!g->probed && !g->testing) {
        // Force detection of the current CPU accelerator.
        GetCurrentCpuAccelerator();
    }

    return g->status_code;
}

void SetCurrentCpuAcceleratorForTesting(CpuAccelerator accel,
                                        AndroidCpuAcceleration status_code,
                                        const char* status) {
    GlobalState *g = &gGlobals;

    g->testing = true;
    g->accel = accel;
    g->status_code = status_code;
    ::snprintf(g->status, sizeof(g->status), "%s", status);
}

std::pair<AndroidHyperVStatus, std::string> GetHyperVStatus() {
#ifndef _WIN32
    // this was easy
    return std::make_pair(ANDROID_HYPERV_ABSENT, "Hyper-V runs only on Windows");
#else // _WIN32
    char vendor_id[16];
    android_get_x86_cpuid_vmhost_vendor_id(vendor_id, sizeof(vendor_id));
    const auto vmType = android_get_x86_cpuid_vendor_vmhost_type(vendor_id);
    if (vmType == VENDOR_VM_HYPERV) {
        // The simple part: there's currently a Hyper-V hypervisor running.
        // Let's find out if we're in a host or guest.
        // Hyper-V has a CPUID function 0x40000003 which returns a set of
        // supported features in ebx register. Ebx[0] is a 'CreatePartitions'
        // feature bit, which is only enabled in host as of now
        uint32_t ebx;
        android_get_x86_cpuid(0x40000003, 0, nullptr, &ebx, nullptr, nullptr);
        if (ebx & 0x1) {
            return std::make_pair(ANDROID_HYPERV_RUNNING,
                    "Hyper-V is enabled");
        } else {
            // TODO: update this part when Hyper-V officially implements
            // nesting support
            return std::make_pair(ANDROID_HYPERV_ABSENT,
                    "Running in a guest Hyper-V VM, Hyper-V is not supported");
        }
    } else if (vmType != VENDOR_VM_NOTVM) {
        // some CPUs may return something strange even if we're not under a VM,
        // so let's double-check it
        if (android_get_x86_cpuid_is_vcpu()) {
            return std::make_pair(ANDROID_HYPERV_ABSENT,
                    "Running in a guest VM, Hyper-V is not supported");
        }
    }

    using android::base::Win32UnicodeString;
    using android::base::PathUtils;

    // Now the hard part: we know Hyper-V is not running. We need to find out if
    // it's installed.
    // The only reliable way of detecting it is to query the list of optional
    // features through the WMI and check if Hyper-V is installed there. But it
    // runs for tens of seconds, and can be even slower under memory pressure.
    // Instead, let's take a shortcut: Hyper-V engine file is vmms.exe. If it's
    // installed it has to be in system32 directory. So we can just check if
    // it's there.
    Win32UnicodeString winPath(MAX_PATH);
    UINT size = ::GetWindowsDirectoryW(winPath.data(), winPath.size() + 1);
    if (size > winPath.size()) {
        winPath.resize(size);
        size = ::GetWindowsDirectoryW(winPath.data(), winPath.size() + 1);
    }
    if (size == 0) {
        // Last chance call
        winPath = L"C:\\Windows";
    } else if (winPath.size() != size) {
        winPath.resize(size);
    }

#ifdef __x86_64__
    const std::string sysPath = PathUtils::join(winPath.toString(), "System32");
#else
    // For the 32-bit application everything's a little bit more complicated:
    // the main Hyper-V executable is 64-bit on 64-bit OS; but we're running
    // under file system redirector which redirects access into 32-bit System32.
    // even more: if we're running under 32-bit Windows, there's no 64-bit
    // directory. So we need to select the proper one here.
    // First, try a symlink which only exists on 64-bit Windows and leads to
    // the native, 64-bit directory
    std::string sysPath = PathUtils::join(winPath.toString(), "Sysnative");

    // check only if path exists: path_is_dir() would fail as it's not a
    // directory but a symlink
    if (!path_exists(sysPath.c_str())) {
        // If it doesn't exist, we're on 32-bit Windows and let's just use
        // the plain old System32
        sysPath = PathUtils::join(winPath.toString(), "System32");
    }
#endif
    const std::string hyperVExe = PathUtils::join(sysPath, "vmms.exe");

    if (path_is_regular(hyperVExe.c_str())) {
        // hyper-v is installed but not running
        return std::make_pair(ANDROID_HYPERV_INSTALLED, "Hyper-V is disabled");
    }

    // not a slightest sign of it
    return std::make_pair(ANDROID_HYPERV_ABSENT, "Hyper-V is not installed");
#endif // _WIN32
}

std::pair<AndroidCpuInfoFlags, std::string> GetCpuInfo() {
    int flags = 0;
    std::string status;

    char vendor_id[13];
    android_get_x86_cpuid_vendor_id(vendor_id, sizeof(vendor_id));
    switch (android_get_x86_cpuid_vendor_id_type(vendor_id)) {
    case VENDOR_ID_AMD:
        flags |= ANDROID_CPU_INFO_AMD;
        status += "AMD CPU\n";
        if (android_get_x86_cpuid_svm_support()) {
            flags |= ANDROID_CPU_INFO_VIRT_SUPPORTED;
            status += "Virtualization is supported\n";
        }
        break;
    case VENDOR_ID_INTEL:
        flags |= ANDROID_CPU_INFO_INTEL;
        status += "Intel CPU\n";
        if (android_get_x86_cpuid_vmx_support()) {
            flags |= ANDROID_CPU_INFO_VIRT_SUPPORTED;
            status += "Virtualization is supported\n";
        }
        break;
    default:
        flags |= ANDROID_CPU_INFO_OTHER;
        status += "Other CPU: ";
        status += vendor_id;
        status += '\n';
        break;
    }

    if (android_get_x86_cpuid_is_vcpu()) {
        flags |= ANDROID_CPU_INFO_VM;
        status += "Inside a VM\n";
    }

    const int osBitness = base::System::get()->getHostBitness();
    const bool is64BitCapable =
            osBitness == 64 || android_get_x86_cpuid_is_64bit_capable();
    if (is64BitCapable) {
        if (osBitness == 32) {
            flags |= ANDROID_CPU_INFO_64_BIT_32_BIT_OS;
            status += "64-bit CPU, 32-bit OS\n";
        } else {
            flags |= ANDROID_CPU_INFO_64_BIT;
            status += "64-bit CPU\n";
        }
    } else {
        flags |= ANDROID_CPU_INFO_32_BIT;
        status += "32-bit CPU\n";
    }

    return std::make_pair(static_cast<AndroidCpuInfoFlags>(flags),
                          std::move(status));
}

std::string CpuAcceleratorToString(CpuAccelerator type) {
    switch (type) {
        case CPU_ACCELERATOR_KVM:
            return "KVM";
        case CPU_ACCELERATOR_HAX:
            return "HAXM";
        case CPU_ACCELERATOR_HVF:
            return "HVF";
        default:
            return "";
    }
}

Version GetCurrentCpuAcceleratorVersion() {
    GlobalState* g = &gGlobals;

    if (!g->probed && !g->testing) {
        // Force detection of the current CPU accelerator.
        GetCurrentCpuAccelerator();
    }
    return (g->accel != CPU_ACCELERATOR_NONE) ? Version(g->version)
                                              : Version::invalid();
}

}  // namespace android
