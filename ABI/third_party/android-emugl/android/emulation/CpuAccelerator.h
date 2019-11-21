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

#include "android/base/Version.h"
#include "android/cpu_accelerator.h"

#include <string>
#include <utility>
#include <stdlib.h>

namespace android {

// The list of CPU emulation acceleration technologies supported by the
// Android emulator.
//  CPU_ACCELERATOR_NONE means no acceleration is supported on this machine.
//
//  CPU_ACCELERATOR_KVM means Linux KVM, which requires a specific driver
//  to be installed and that /dev/kvm is properly accessible by the current
//  user.
//
//  CPU_ACCELERATOR_HAX means Intel's Hardware Accelerated eXecution,
//  which can be installed on Windows and OS X machines running on an
//  Intel processor.
//
//  CPU_ACCELERATOR_HVF means Apple's Hypervisor.framework, which
//  requres an Intel Mac running OS X 10.10+.
//
enum CpuAccelerator {
    CPU_ACCELERATOR_NONE = 0,
    CPU_ACCELERATOR_KVM,
    CPU_ACCELERATOR_HAX,
    CPU_ACCELERATOR_HVF,
    CPU_ACCELERATOR_HYPERV,
    CPU_ACCELERATOR_MAX,
};

// Return the CPU accelerator technology usable on the current machine.
// This only returns a non-CPU_ACCELERATOR_NONE if corresponding accelerator
// can be used properly. Otherwise it will return CPU_ACCELERATOR_NONE.
CpuAccelerator  GetCurrentCpuAccelerator();
void ResetCurrentCpuAccelerator(CpuAccelerator accel);

// Returns whether or not the accelerator |type| is suppored
// on the current system.
bool GetCurrentAcceleratorSupport(CpuAccelerator type);

// Return an ASCII string describing the state of the current CPU
// acceleration on this machine. If GetCurrentCpuAccelerator() returns
// CPU_ACCELERATOR_NONE this will contain a small explanation why
// the accelerator cannot be used.
std::string GetCurrentCpuAcceleratorStatus();

// Return the version for the CPU accelerator technology usable
// on the current machine.
android::base::Version GetCurrentCpuAcceleratorVersion();

// Convert CpuAccelerator to string type
std::string CpuAcceleratorToString(CpuAccelerator type);

// Return an status code describing the state of the current CPU
// acceleration on this machine. If GetCurrentCpuAccelerator() returns
// CPU_ACCELERATOR_NONE this will contain a small explanation why
// the accelerator cannot be used.
AndroidCpuAcceleration GetCurrentCpuAcceleratorStatusCode();

// For unit testing/debugging purpose only, must be called before
// GetCurrentCpuAccelerator().
void SetCurrentCpuAcceleratorForTesting(CpuAccelerator accel,
                                        AndroidCpuAcceleration status_code,
                                        const char* status);

// Returns the Hyper-V configuration of the current system
// and a short message describing it.
std::pair<AndroidHyperVStatus, std::string> GetHyperVStatus();

// Returns a set of AndroidCpuInfoFlags describing the CPU capabilities
// (and a text explanation as well)
std::pair<AndroidCpuInfoFlags, std::string> GetCpuInfo();

}  // namespace android
