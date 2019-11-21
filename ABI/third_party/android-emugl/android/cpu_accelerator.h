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

#pragma once

#include <stdbool.h>

#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

// don't change these numbers
// Android Studio depends on them
typedef enum {
    ANDROID_CPU_ACCELERATION_READY                    = 0, // Acceleration is available
    ANDROID_CPU_ACCELERATION_NESTED_NOT_SUPPORTED     = 1, // HAXM doesn't support nested VM
    ANDROID_CPU_ACCELERATION_INTEL_REQUIRED           = 2, // HAXM requires GeniuneIntel processor
    ANDROID_CPU_ACCELERATION_NO_CPU_SUPPORT           = 3, // CPU doesn't support required features (VT-x or SVM)
    ANDROID_CPU_ACCELERATION_NO_CPU_VTX_SUPPORT       = 4, // CPU doesn't support VT-x
    ANDROID_CPU_ACCELERATION_NO_CPU_NX_SUPPORT        = 5, // CPU doesn't support NX
    ANDROID_CPU_ACCELERATION_ACCEL_NOT_INSTALLED      = 6, // KVM/HAXM package not installed
    ANDROID_CPU_ACCELERATION_ACCEL_OBSOLETE           = 7, // HAXM package is obsolete
    ANDROID_CPU_ACCELERATION_DEV_NOT_FOUND            = 8, // /dev/kvm is not found: VT disabled in BIOS or KVM kernel module not loaded
    ANDROID_CPU_ACCELERATION_VT_DISABLED              = 9, // HAXM is installed but VT disabled in BIOS
    ANDROID_CPU_ACCELERATION_NX_DISABLED              = 10, // HAXM is installed but NX disabled in BIOS
    ANDROID_CPU_ACCELERATION_DEV_PERMISSION           = 11, // /dev/kvm or HAXM device: permission denied
    ANDROID_CPU_ACCELERATION_DEV_OPEN_FAILED          = 12, // /dev/kvm or HAXM device: open failed
    ANDROID_CPU_ACCELERATION_DEV_IOCTL_FAILED         = 13, // /dev/kvm or HAXM device: ioctl failed
    ANDROID_CPU_ACCELERATION_DEV_OBSOLETE             = 14, // KVM or HAXM supported API is too old
    ANDROID_CPU_ACCELERATION_HYPERV_ENABLED           = 15, // HyperV must be disabled
    ANDROID_CPU_ACCELERATION_ERROR                    = 138, // Some other error occurred
} AndroidCpuAcceleration;

// ditto
typedef enum {
    ANDROID_HYPERV_ABSENT       = 0,    // No hyper-V found
    ANDROID_HYPERV_INSTALLED    = 1,    // Hyper-V is installed but not running
    ANDROID_HYPERV_RUNNING      = 2,    // Hyper-V is up and running
    ANDROID_HYPERV_ERROR        = 100,  // Failed to detect status
} AndroidHyperVStatus;

// For any possible Android Studio Hypervisor.framework detection
typedef enum {
    ANDROID_HVF_UNSUPPORTED = 0, // No Hypervisor.framework support on host system
    ANDROID_HVF_SUPPORTED = 1, // Hypervisor.framework is supported
    ANDROID_HVF_ERROR = 100, // Failed to detect status
} AndroidHVFStatus;

// +1, cpu info
typedef enum {
    ANDROID_CPU_INFO_FAILED = 0,    // this is the value to return if something
                                    // went wrong

    ANDROID_CPU_INFO_AMD = 1 << 0,       // AMD CPU
    ANDROID_CPU_INFO_INTEL = 1 << 1,     // Intel CPU
    ANDROID_CPU_INFO_OTHER = 1 << 2,     // Other CPU manufacturer
    ANDROID_CPU_INFO_VM = 1 << 3,        // Running in a VM
    ANDROID_CPU_INFO_VIRT_SUPPORTED = 1 << 4,    // CPU supports
                                                 // virtualization technologies

    ANDROID_CPU_INFO_32_BIT = 1 << 5,           // 32-bit CPU, really!
    ANDROID_CPU_INFO_64_BIT_32_BIT_OS = 1 << 6, // 32-bit OS on a 64-bit CPU
    ANDROID_CPU_INFO_64_BIT = 1 << 7,           // 64-bit CPU

} AndroidCpuInfoFlags;

/* Returns ANDROID_CPU_ACCELERATION_READY if CPU acceleration is
 *  possible on this machine.  If |status| is not NULL, on exit,
 * |*status| will be set to a heap-allocated string describing
 * the status of acceleration, to be freed by the caller.
 */
AndroidCpuAcceleration androidCpuAcceleration_getStatus(char** status);

typedef enum {
    ANDROID_CPU_ACCELERATOR_NONE = 0,
    ANDROID_CPU_ACCELERATOR_KVM,
    ANDROID_CPU_ACCELERATOR_HAX,
    ANDROID_CPU_ACCELERATOR_HVF,
    ANDROID_CPU_ACCELERATOR_HYPERV,
    ANDROID_CPU_ACCELERATOR_MAX,
} AndroidCpuAccelerator;

/* Returns the auto-selected CPU accelerator. */
AndroidCpuAccelerator androidCpuAcceleration_getAccelerator();

/* Returns support status of the cpu accelerator |type| on the current machine. */
bool androidCpuAcceleration_isAcceleratorSupported(AndroidCpuAccelerator type);

/* Resets the current cpu accelerator to reflect current status. */
void androidCpuAcceleration_resetCpuAccelerator(AndroidCpuAccelerator type);

ANDROID_END_HEADER
