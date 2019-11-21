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

#include <sys/types.h>

namespace android {
namespace base {

// Close a socket, preserves errno to ensure it can be used in
// error-handling code paths.
void socketClose(int socket);

// Try to receive up to |bufferLen| bytes from |socket| into |buffer|.
// Return the number of bytes actually read, 0 in case of disconnection,
// or -1/errno in case of error. Note that this loops around EINTR as
// a convenience.
ssize_t socketRecv(int socket, void* buffer, size_t bufferLen);

// Try to send up to |bufferLen| bytes to |socket| from |buffer|.
// Return the number of bytes actually sent, 0 in case of disconnection,
// or -1/errno in case of error. Note that this loops around EINTR as
// a convenience. Also, this will *not* generate a SIGPIPE signal when
// writing to a broken pipe (but errno will be set to EPIPE).
ssize_t socketSend(int socket, const void* buffer, size_t bufferLen);

// Same as socketSend() but loop around transient writes.
// Returns true if all bytes were sent, false otherwise.
bool socketSendAll(int socket, const void* buffer, size_t bufferLen);

// Same as socketRecv() but loop around transient reads.
// Returns ture if all bytes were received, false otherwise.
bool socketRecvAll(int socket, void* buffer, size_t bufferLen);

// Shutdown all writes to a socket.
void socketShutdownWrites(int socket);

// Shutdown all reads from a socket.
void socketShutdownReads(int socket);

// Set the socket descriptor |socket| to non-blocking mode.
void socketSetNonBlocking(int socket);

// Set the socket descriptor |socket| to blocking mode.
void socketSetBlocking(int socket);

// Disable TCP Nagle algorithm for |socket|.
void socketSetNoDelay(int socket);

// Bind and listen on TCP |port| on loopback interface (i.e. 127.0.0.1).
// Return new socket on success, or -1/errno on error.
int socketTcp4LoopbackServer(int port);

// Bind and listen on TCP |port| on IPv6 loopback interface (i.e. ::1).
// Return new socket on success, or -1/errno on error.
int socketTcp6LoopbackServer(int port);

// Connect to TCP |port| on loopback interface (i.e. 127.0.0.1).
// Return new socket on success, or -1/errno on error.
int socketTcp4LoopbackClient(int port);

// Connecto TCP |port| on IPV6 loopback interface (i.e. ::1).
// Return new socket on success, or -1/errno on error.
int socketTcp6LoopbackClient(int port);

// Accept a connection on server |socket|, and return the new connection
// socket descriptor, or -1/errno on error.
int socketAcceptAny(int socket);

// Create a pair of non-blocking sockets connected to each other, this is
// equivalent to calling the Unix function:
//     socketpair(AF_LOCAL, SOCK_STREAM, 0, &fds);
//
// On Windows this will use a pair of TCP loopback sockets instead.
// Return 0 in success, or -1/errno on error.
int socketCreatePair(int *s1, int* s2);

// Create a new TCP-based socket. At the moment, this should only be used
// for unit-testing.
int socketCreateTcp4();
int socketCreateTcp6();

// Return the port number of a TCP or UDP socket, or -1/errno otherwise.
int socketGetPort(int socket);

// Return the port number of a TCP socket peer, if available, or
// -1/errno otherwise.
int socketGetPeerPort(int socket);

}  // namespace base
}  // namespace android
