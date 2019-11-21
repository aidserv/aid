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

namespace android {
namespace wear {
namespace testing {

// Bind to a random TCP loopback port.
// On success, return new server socket descriptor, and sets |*hostPort|
// to the corresponding port number. On failure, return -1/errno.
int testStartMockServer(int* hostPort);

// Run a mock host ADB server instance which will be used to interact
// with the WearAgent implementation during unit testing.
//
// |adbServerSocket| is the server socket descriptor for ADB connection
// that will be used by the agent.
// |consoleServerSocket| is the console server socket for console
// connections that will be used by the agent if it thinks it is talking
// to an emulator, instead of a real device. This is ignored if |usbPhone|
// is true.
// |wearDevice| is the serial number of a simulated wear device or emulator.
// |phoneDevice| is the serial number of a simulated compatible device or
// emulator to be paired-up with |wearDevice|
// |usbPhone| is true to indicate that |phoneDevice| is a simulated device,
// and false to indicate that it is a simulated emulator.
//
// Return true on success, false/errno otherwise.
bool testRunMockAdbServer(int adbServerSocket,
                          int consoleServerSocket,
                          const char* wearDevice,
                          const char* phoneDevice,
                          bool usbPhone);

// Send a given zero-terminated |message| to socket |socketFd|.
// On success, return true, on error return false/errno.
bool testSendToSocket(int socketFd, const char* message);

// Receive a message of exactly |size| bytes from socket |socketFd| and
// store the data into |ptr|. On success, return true. On failure return
// false/errno.
bool testReceiveMessageFromSocket(int socketFd, char* ptr, int size);

// Receive a message from socket |socketFd|.
// If |header| is 1, then this reads a 4-byte hexchar header giving the
// size of the message, then the payload. Otherwise, read data until
// the socket is disconnected.
// The received data must match |message|.
// Return true on success, false/errno on failure.
bool testExpectMessageFromSocket(int socketFd,
                                 const char* message,
                                 int header = 1);

} // namespace testing
} // namespace wear
} // namespace android
