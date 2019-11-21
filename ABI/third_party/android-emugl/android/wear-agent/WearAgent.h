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

#include "android/base/Compiler.h"
#include "android/base/async/Looper.h"

// This class establishes a connection between one Android Wear and one
// compatible phone, so that the wear can get notifications automatically
// from the phone. The phone could be emulated phone or a real phone.
//
// Usage is simply:
//
//     WearAgent agent(looper);
//
// Then simply run the |looper| as usual.
//
// When |agent| falls out of scope before the looper stops, any on-going
// pairing activity will be aborted. In addition, the looper should not be
// destroyed when the agent is destroyed.

namespace android {

namespace base {
class Looper;
}  // namespace base

namespace wear {

class WearAgentImpl;

class WearAgent {
public:
    // Create a new Android Wear agent instance.
    //
    // |looper| is a Looper instance that is used to by the agent to process
    // pairing requests asynchronously.
    //
    // |adbHostPort| is the localhost TCP port number used to talk to
    // the ADB server on the host.
    //
    // Creating the instance doesn't do anything, one has to run the looper
    // to ensure that the agent works.
    WearAgent(::android::base::Looper* looper, int adbHostPort = 5037);

    // Destroy a given Android Wear agent instance. This removes it from
    // its looper. NOTE: This must happen _before_ the looper is destroyed.
    ~WearAgent();

private:
    DISALLOW_COPY_AND_ASSIGN(WearAgent);

    WearAgentImpl*  mWearAgentImpl;
};

}  // namespace wear
}  // namespace android
