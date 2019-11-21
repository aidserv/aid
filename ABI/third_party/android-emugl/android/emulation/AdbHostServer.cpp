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

#include "android/emulation/AdbHostServer.h"

#include "android/base/Log.h"
#include "android/base/sockets/ScopedSocket.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/base/StringFormat.h"
#include "android/base/StringView.h"
#include "android/base/system/System.h"

#include <string>
#include <stdlib.h>

namespace android {
namespace emulation {

using android::base::ScopedSocket;
using android::base::StringFormat;

bool AdbHostServer::notify(int adbEmulatorPort, int adbClientPort) {
    // First connect to ADB server.
    ScopedSocket socket(android::base::socketTcp6LoopbackClient(adbClientPort));
    if (!socket.valid()) {
        socket.reset(android::base::socketTcp4LoopbackClient(adbClientPort));
    }
    if (!socket.valid()) {
        // This can happen frequently when there is no ADB Server running
        // on the host, so don't complain with a warning or error message.
        return false;
    }
    // Send the special message
    auto message = StringFormat("host:emulator:%d", adbEmulatorPort);
    auto handshake = StringFormat("%04x%s", message.size(), message.c_str());
    LOG(VERBOSE) << "Attempting to notify adb host server with |" << handshake
                 << "|";
    return android::base::socketSendAll(socket.get(), handshake.c_str(),
                                        handshake.size());
}

int AdbHostServer::getClientPort() {
    int clientPort = kDefaultAdbClientPort;
    const android::base::StringView kVarName = "ANDROID_ADB_SERVER_PORT";
    std::string env = android::base::System::get()->envGet(kVarName);
    if (!env.empty()) {
        long port = strtol(env.c_str(), NULL, 0);
        if (port <= 0 || port >= 65536) {
            LOG(ERROR) << "Env. var " << kVarName << " must be a number "
                       << "in 1..65535 range. Got " << port;
            return -1;
        }
        clientPort = static_cast<int>(port);
    }
    return clientPort;
}

}  // namespace emulation
}  // namespace android
