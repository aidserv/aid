// Copyright 2014 The Android Open Source Project
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

// C interface to android/wear-agent/WearAgent.h

#include "android/utils/compiler.h"
#include "android/utils/looper.h"

ANDROID_BEGIN_HEADER

// Start the agent, using |looper| to perform asynchronous tasks.
void android_wear_agent_start(Looper* looper);

// Stop the agent.
void android_wear_agent_stop(void);

ANDROID_END_HEADER
