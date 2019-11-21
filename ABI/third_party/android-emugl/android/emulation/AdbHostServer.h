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

#pragma once

namespace android {
namespace emulation {

// Utility class to provide static methods to interact with the ADB Server
// running on the  host.
struct AdbHostServer {
    // Notify the host server, if any, that an emulator instance is listening
    // for connections on |adbEmulatorPort| localhost TCP port. |adbClientPort|
    // is the localhost TCP port the ADB server is listening on for client
    // connections where the message should be sent. Except during
    // unit-testing, this should be the result of getClientPort(). Return true
    // on success, false/errno on error.
    static bool notify(int adbEmulatorPort, int adbClientPort);

    // Default ADB client port value.
    static constexpr int kDefaultAdbClientPort = 5037;

    // Return the ADB client port to use for this emulator instance, this is
    // either |kDefaultAdbClientPort| or an over-riden value from an environment
    // variable.
    static int getClientPort();
};

}  // namespace emulation
}  // namespace android
