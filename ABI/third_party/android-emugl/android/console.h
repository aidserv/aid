// Copyright 2015 The Android Open Source Project
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

#pragma once

#include "android/emulation/control/battery_agent.h"
#include "android/emulation/control/car_data_agent.h"
#include "android/emulation/control/finger_agent.h"
#include "android/emulation/control/libui_agent.h"
#include "android/emulation/control/location_agent.h"
#include "android/emulation/control/http_proxy_agent.h"
#include "android/emulation/control/net_agent.h"
#include "android/emulation/control/telephony_agent.h"
#include "android/emulation/control/user_event_agent.h"
#include "android/emulation/control/vm_operations.h"
#include "android/emulation/control/window_agent.h"
#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

// A macro used to list all the agents used by the Android Console.
// The macro takes a parameter |X| which must be a macro that takes two
// parameter as follows:  X(type, name), where |type| is the agent type
// name, and |name| is a field name. See usage below to declare
// AndroidConsoleAgents.
#define ANDROID_CONSOLE_AGENTS_LIST(X)    \
    X(QAndroidBatteryAgent, battery)      \
    X(QAndroidEmulatorWindowAgent, emu)   \
    X(QAndroidFingerAgent, finger)        \
    X(QAndroidLocationAgent, location)    \
    X(QAndroidHttpProxyAgent, proxy)      \
    X(QAndroidTelephonyAgent, telephony)  \
    X(QAndroidUserEventAgent, user_event) \
    X(QAndroidVmOperations, vm)           \
    X(QAndroidNetAgent, net)              \
    X(QAndroidLibuiAgent, libui)          \
    X(QCarDataAgent, car)

// A structure used to group pointers to all agent interfaces used by the
// Android console.
#define ANDROID_CONSOLE_DEFINE_POINTER(type,name) const type* name;
typedef struct AndroidConsoleAgents {
    ANDROID_CONSOLE_AGENTS_LIST(ANDROID_CONSOLE_DEFINE_POINTER)
} AndroidConsoleAgents;

// Generic entry point to start an android console.
// QEMU implementations should populate |*agents| with QEMU specific
// functions. Takes ownership of |agents|.
extern int android_console_start(int port, const AndroidConsoleAgents* agents);

ANDROID_END_HEADER
