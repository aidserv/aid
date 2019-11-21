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

#include "android/wear-agent/android_wear_agent.h"

#include "android/base/Log.h"
#include "android/wear-agent/WearAgent.h"

namespace {

typedef ::Looper CLooper;

android::wear::WearAgent* sAgent = NULL;

}  // namespace

void android_wear_agent_start(CLooper* looper) {
    DCHECK(looper);
    // TODO(digit): Make this thread-safe?
    if (!sAgent) {
        sAgent = new android::wear::WearAgent(
                reinterpret_cast<android::base::Looper*>(looper));
    }
}

void android_wear_agent_stop(void) {
    DCHECK(sAgent);
    android::wear::WearAgent* agent = sAgent;
    sAgent = NULL;
    delete agent;
}
