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

#include "android/cpu_accelerator.h"

// This source acts as a small C++ -> C bridge between android/emulation/
// and android-qemu1-glue/main.c

#include "android/base/misc/StringUtils.h"
#include "android/emulation/CpuAccelerator.h"
#include "android/cpu_accelerator.h"

#include "android/utils/system.h"

extern "C" {

AndroidCpuAcceleration androidCpuAcceleration_getStatus(
        char** status_p) {
    AndroidCpuAcceleration result = android::GetCurrentCpuAcceleratorStatusCode();

    if (status_p) {
        *status_p = android::base::strDup(
                android::GetCurrentCpuAcceleratorStatus());
    }

    return result;
}

AndroidCpuAccelerator androidCpuAcceleration_getAccelerator() {
    return (AndroidCpuAccelerator)android::GetCurrentCpuAccelerator();
}

bool androidCpuAcceleration_isAcceleratorSupported(AndroidCpuAccelerator type) {
    return android::GetCurrentAcceleratorSupport((android::CpuAccelerator)type);
}

void androidCpuAcceleration_resetCpuAccelerator(AndroidCpuAccelerator type) {
    android::ResetCurrentCpuAccelerator((android::CpuAccelerator)type);
}

} // extern "C"
