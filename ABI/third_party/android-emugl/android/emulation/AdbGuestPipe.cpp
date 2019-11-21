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

#include "android/emulation/AdbGuestPipe.h"

#include "android/base/async/AsyncSocketServer.h"
#include "android/base/async/Looper.h"
#include "android/base/async/ScopedSocketWatch.h"
#include "android/base/async/ThreadLooper.h"
#include "android/base/Log.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/base/StringView.h"
#include "android/emulation/VmLock.h"
#include "android/globals.h"
#include "android/utils/debug.h"

#include <algorithm>
#include <string>

#include <assert.h>

#define DEBUG 0

#if DEBUG >= 1
#include <stdio.h>
#define D(...) fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n")
#else
#define D(...) (void)0
#endif

#if DEBUG >= 2
#define DD(...) fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n")
#else
#define DD(...) (void)0
#endif

#define E(...) fprintf(stderr, "ERROR:" __VA_ARGS__), fprintf(stderr, "\n")

#define DINIT(...) do { if (DEBUG || VERBOSE_CHECK(init)) dprint(__VA_ARGS__); } while (0)

namespace android {
namespace emulation {

// Technical note: full state transition diagram
//
// State::WaitingForGuestAcceptCommand:
//   onGuestClose() -> State::ClosedByGuest
//   receive bad command bytes -> State::ClosedByHost
//   receive complete command -> State::WaitingForHostAdbConnection or
//                               State::SendingAcceptReplyOk
//   onHostConnection -> State::WaitingForGuestAcceptCommand (self)
//
// State::WaitingForHostAdbConnection:
//   onGuestClose -> State::ClosedByGuest
//   onHostConnection -> State::SendingAcceptReplyOk
//
// State::SendingAcceptReplyOk:
//   onGuestClose -> State::ClosedByGuest
//   all bytes sent -> State::WaitingForGuestStartCommand
//
// State::WaitingForGuestStartCommand:
//   onGuestClose -> State::ClosedByGuest
//   receive bad command bytes -> State::ClosedByHost
//   receive complete command -> State::ProxyingData
//
// State::ProxyingData:
//   onGuestClose -> State::ClosedByGuest
//   on host disconnect -> State::ClosedByHost
//   data in or out -> State;:ProxyingData (self)
//
// State::ClosedByHost:
//   onGuestClose -> State::ClosedByGuest
//
// State::ClosedByGuest:
//   close host socket ->  State::ClosedByGuest
//
// How multiple active pipes are supported
// First of all, the guest adbd is driving the connection.
// in the guest, adbd is running the following loop (not exact code)
/*
   for (;;) {
        open_adb_pipe();//this will tell emulator-5554 (for example) to listen
                        //at 5555 (i.e., 5554+1) for adb server connection

        wait_for_ok_replay();//emulator sends "OK" when the pipe is connected with adb server

        register_the_connected_pipe();

        open_another_adb_pipe();//previously, emulator will not listen again, as there is
                                //one activepipe already; now it will just listen again
                                //now this new pipe will trigger emulator to listen again,
        wait_for_ok_replay();//emulator sends "OK" when the pipe is connected with adb server
        register_the_connected_pipe();
   }
*/

// The Service object has a vector of pipes |mPipes| waiting
// for host connection, in FIFO mode.
// Whenever there is a connection from host, Service
// will remove the first pipe in the queue and wait
// for another new connection. Each connected pipe runs
// independently from each other once connected by
// Service.
//
// Note that Service runs in the main-loop thread
// and AdbGuestPipe runs in other cpu threads;
// on the guest side, it always starts another pipe
// after the current one is connected with host side
// already: there is no race condition there.
//
// Previously, the new pipe has to wait for the existing
// active pipe to disconnect before becoming active;
// now, it only need to wait for the current active pipe to
// become fully connected. Once that happens, the new
// pipe is ready to connect with host.
//
// One example might help explain better:
// Assume there is only one emulator-5554.
//
// Suppose host adb server is running at 5037, and it tries to
// connect to 5555 port, but before there is any pipe,
// the 5555 port is not listening yet. When guest adbd
// starts, a new pipe is created and it sends "accept"
// to emulator, upon receiving "accept", emulator starts
// listening at 5555, and accept a socket from adb server
// running at 5037. And the first pipe is connected now. Once
// guest side adbd receives the "ok" from host ("ok" means
// fully connected), it starts another new pipe, this new pipe
// will cause emulator to listen again for adb server connection
// (previously, it wont listen again because there is one
//  active pipe running already).
//
// Assume there is another adb server running at 5038
// (previously, it cannot connect to 5555 port, because there
// is an active pipe running already, and emualtor wll stop
// listening at 5555 because of that), it
// will connect to this new pipe right away. Once the new pipe
// is connected, guest adbd will start another one immediately
// --there is no limit on how many pipes the guest adbd can start
// (of course, it may run out of fds)
//
// This way, multiple active pipes can run without affecting
// each other.

using FdWatch = android::base::Looper::FdWatch;
using android::base::ScopedSocketWatch;
using android::base::StringView;

#if DEBUG >= 2
static int bufferBytes(const AndroidPipeBuffer* buffers, int count) {
    int result = 0;
    for (int n = 0; n < count; ++n) {
        result += buffers[n].size;
    }
    return result;
}
#endif

AndroidPipe* AdbGuestPipe::Service::create(void* mHwPipe, const char* args) {
    auto pipe = new AdbGuestPipe(mHwPipe, this, mHostAgent);
    onPipeOpen(pipe);
    DD("%s: [%p] created", __func__, pipe);
    return pipe;
}

void AdbGuestPipe::Service::removeAdbGuestPipe(AdbGuestPipe* pipe) {
    mPipes.erase(std::remove(mPipes.begin(), mPipes.end(), pipe), mPipes.end());
}

void AdbGuestPipe::Service::onHostConnection(ScopedSocket&& socket) {
    // There must be no active pipe yet, but at least one waiting
    // for activation in mPipes.
    // We have one connection from adb sever, stop listening for now.
    mHostAgent->stopListening();
    AdbGuestPipe* activePipe = searchForActivePipe();
    CHECK(activePipe != nullptr);
    activePipe->onHostConnection(std::move(socket));
}

void AdbGuestPipe::Service::onPipeOpen(AdbGuestPipe* pipe) {
    mPipes.push_back(pipe);
}

void AdbGuestPipe::Service::onPipeClose(AdbGuestPipe* pipe) {
    removeAdbGuestPipe(pipe);
    if (mPipes.empty()) {
        mHostAgent->stopListening();
    }
    delete pipe;
}

AdbGuestPipe* AdbGuestPipe::Service::searchForActivePipe() {
    const auto pipeIt = std::find_if(
            mPipes.begin(), mPipes.end(), [](const AdbGuestPipe* pipe) {
                return pipe->mState == State::WaitingForHostAdbConnection;
            });
    if (pipeIt != mPipes.end()) {
        AdbGuestPipe* activePipe = *pipeIt;
        removeAdbGuestPipe(activePipe);
        return activePipe;
    }
    return nullptr;
}

AdbGuestPipe::~AdbGuestPipe() {
    DD("%s: [%p] destroyed", __func__, this);
    CHECK(mState == State::ClosedByGuest);
}

void AdbGuestPipe::onGuestClose(PipeCloseReason reason) {
    DD("%s: [%p]", __func__, this);
    mState = State::ClosedByGuest;
    DINIT("%s: [%p] Adb closed by guest",__func__, this);
    mHostSocket.reset();
    service()->onPipeClose(this);  // This deletes the instance.
}

unsigned AdbGuestPipe::onGuestPoll() const {
    DD("%s: [%p]", __func__, this);
    unsigned result = 0;
    switch (mState) {
        case State::WaitingForGuestAcceptCommand:
        case State::WaitingForGuestStartCommand:
            result = PIPE_POLL_OUT;
            break;

        case State::SendingAcceptReplyOk:
            result = PIPE_POLL_IN;
            break;

        case State::ProxyingData: {
            unsigned flags = mHostSocket->poll();
            if (flags & FdWatch::kEventRead) {
                result |= PIPE_POLL_IN;
            }
            if (flags & FdWatch::kEventWrite) {
                result |= PIPE_POLL_OUT;
            }
            break;
        }

        case State::ClosedByHost:
            result |= PIPE_POLL_HUP;
            break;

        default:;
    }
    return result;
}

int AdbGuestPipe::onGuestRecv(AndroidPipeBuffer* buffers, int numBuffers) {
    DD("%s: [%p] numBuffers=%d bytes=%d state=%s", __func__, this, numBuffers,
       bufferBytes(buffers, numBuffers), toString(mState));
    if (mState == State::ProxyingData) {
        // Common case, proxy-ing the data from the host to the guest.
        return onGuestRecvData(buffers, numBuffers);
    } else if (guest_data_partition_mounted == 0 && mPlayStoreImage) {
        return PIPE_ERROR_AGAIN;
    } else if (mState == State::SendingAcceptReplyOk) {
        // The guest is receiving the 'ok' reply.
        return onGuestRecvReply(buffers, numBuffers);
    } else if (mState == State::WaitingForHostAdbConnection ||
               mState == State::WaitingForGuestStartCommand) {
        // Not ready yet to send data to the guest.
        return PIPE_ERROR_AGAIN;
    } else {
        if (mState != State::ClosedByHost) {
            // Invalid state !!!
            LOG(ERROR) << "Invalid state: " << toString(mState);
        }
        return PIPE_ERROR_IO;
    }
}

int AdbGuestPipe::onGuestSend(const AndroidPipeBuffer* buffers,
                              int numBuffers) {
    DD("%s: [%p] numBuffers=%d bytes=%d state=%s", __func__, this, numBuffers,
       bufferBytes(buffers, numBuffers), toString(mState));
    if (mState == State::ProxyingData) {
        // Common-case, proxy-ing the data from the guest to the host.
        return onGuestSendData(buffers, numBuffers);
    } else if (mState == State::WaitingForGuestAcceptCommand ||
               mState == State::WaitingForGuestStartCommand) {
        // Waiting command bytes from the guest.
        return onGuestSendCommand(buffers, numBuffers);
    } else {
        if (mState != State::ClosedByHost) {
            // Invalid state !!!
            LOG(ERROR) << "Invalid state: " << toString(mState);
        }
        return PIPE_ERROR_IO;
    }
}

void AdbGuestPipe::onGuestWantWakeOn(int flags) {
    DD("%s: [%p] flags=%x (%d)", __func__, this, (unsigned)flags, flags);
    if (flags & PIPE_WAKE_READ) {
        if (mHostSocket.get()) {
            mHostSocket->wantRead();
        }
    }
    if (flags & PIPE_WAKE_WRITE) {
        if (mHostSocket.get()) {
            mHostSocket->wantWrite();
        }
    }
}

void AdbGuestPipe::onHostConnection(ScopedSocket&& socket) {
    DD("%s: [%p] host connection", __func__, this);
    CHECK(mState <= State::WaitingForHostAdbConnection);
    android::base::socketSetNonBlocking(socket.get());
    // socketSetNoDelay() reduces the latency of sending data, at the cost
    // of creating more TCP packets on the connection. It's useful when
    // doing lots of small send() calls, like the ADB protocol requires.
    // And since this is on localhost, the packet increase should not be
    // noticeable.
    android::base::socketSetNoDelay(socket.get());

    mHostSocket.reset(android::base::ThreadLooper::get()->createFdWatch(
            socket.release(),
            [](void* opaque, int fd, unsigned events) {
                static_cast<AdbGuestPipe*>(opaque)->onHostSocketEvent(events);
            },
            this));

    DD("%s: [%p] sending reply", __func__, this);
    setReply("ok", State::SendingAcceptReplyOk);
    signalWake(PIPE_WAKE_READ);
}

// static
const char* AdbGuestPipe::toString(AdbGuestPipe::State state) {
    switch (state) {
        case State::WaitingForGuestAcceptCommand:
            return "WaitingForGuestAcceptCommand";
        case State::WaitingForHostAdbConnection:
            return "WaitingForHostAdbConnection";
        case State::SendingAcceptReplyOk:
            return "SendingAcceptReplyOk";
        case State::WaitingForGuestStartCommand:
            return "WaitingForGuestStartCommand";
        case State::ProxyingData:
            return "ProxyingData";
        case State::ClosedByGuest:
            return "ClosedByGuest";
        case State::ClosedByHost:
            return "ClosedByHost";
        default:
            return "Unknown";
    }
}

// Called whenever an i/o event occurs on the host socket.
void AdbGuestPipe::onHostSocketEvent(unsigned events) {
    DD("%s: [%p] events=%x (%u)", __func__, this, events, events);
    int wakeFlags = 0;
    if ((events & FdWatch::kEventRead) != 0) {
        wakeFlags |= PIPE_WAKE_READ;
        mHostSocket->dontWantRead();
    }
    if ((events & FdWatch::kEventWrite) != 0) {
        wakeFlags |= PIPE_WAKE_WRITE;
        mHostSocket->dontWantWrite();
    }
    if (wakeFlags) {
        signalWake(wakeFlags);
    }
}

int AdbGuestPipe::onGuestRecvData(AndroidPipeBuffer* buffers, int numBuffers) {
    DD("%s: [%p] numBuffers=%d bytes=%d", __func__, this, numBuffers,
        bufferBytes(buffers, numBuffers));
    CHECK(mState == State::ProxyingData);
    int result = 0;
    while (numBuffers > 0) {
        uint8_t* data = buffers[0].data;
        size_t dataSize = buffers[0].size;
        DD("%s: [%p] dataSize=%d", __func__, this, (int)dataSize);
        while (dataSize > 0) {
            ssize_t len;
            {
                ScopedVmUnlock unlockBql;
                len = android::base::socketRecv(mHostSocket->fd(),
                                                data, dataSize);
            }
            if (len > 0) {
                data += len;
                dataSize -= len;
                result += static_cast<int>(len);
                if (dataSize != 0) {
                    return result; // got less than requested -> no more data.
                } else {
                    break;
                }
            }
            if (len < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                if (result == 0) {
                    mHostSocket->dontWantRead();
                    return PIPE_ERROR_AGAIN;
                }
            } else {
                // End of stream or i/o error means the guest has closed
                // the connection.
                if (result == 0) {
                    mHostSocket.reset();
                    mState = State::ClosedByHost;
                    DINIT("%s: [%p] Adb closed by host",__func__, this);
                    return PIPE_ERROR_IO;
                }
            }
            // Some data was received so report it, the guest will have
            // to try again to get an error.
            return result;
        }
        buffers++;
        numBuffers--;
    }
    return result;
}

int AdbGuestPipe::onGuestSendData(const AndroidPipeBuffer* buffers,
                                  int numBuffers) {
    DD("%s: [%p] numBuffers=%d bytes=%d", __func__, this, numBuffers,
        bufferBytes(buffers, numBuffers));
    CHECK(mState == State::ProxyingData);
    int result = 0;
    while (numBuffers > 0) {
        const uint8_t* data = buffers[0].data;
        size_t dataSize = buffers[0].size;
        while (dataSize > 0) {
            ssize_t len;
            {
                ScopedVmUnlock unlockBql;
                len = android::base::socketSend(mHostSocket->fd(),
                                                data, dataSize);
            }
            if (len > 0) {
                data += len;
                dataSize -= len;
                result += static_cast<int>(len);
                if (dataSize != 0) {
                    return result; // sent less than requested -> no more room.
                } else {
                    break;
                }
            }
            if (len < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                if (result == 0) {
                    mHostSocket->dontWantWrite();
                    return PIPE_ERROR_AGAIN;
                }
            } else {
                // End of stream or i/o error means the guest has closed
                // the connection.
                if (result == 0) {
                    mHostSocket.reset();
                    mState = State::ClosedByHost;
                    DINIT("%s: [%p] Adb closed by host",__func__, this);
                    return PIPE_ERROR_IO;
                }
            }
            // Some data was received so report it, the guest will have
            // to try again to get an error.
            return result;
        }
        buffers++;
        numBuffers--;
    }
    return result;
}

int AdbGuestPipe::onGuestRecvReply(AndroidPipeBuffer* buffers, int numBuffers) {
    // Sending reply to the guest.
    CHECK(mState == State::SendingAcceptReplyOk);
    DD("%s: [%p] numBuffers=%d", __func__, this, numBuffers);
    int result = 0;
    while (numBuffers > 0) {
        uint8_t* data = buffers[0].data;
        size_t dataSize = buffers[0].size;
        while (dataSize > 0) {
            size_t avail = std::min(mBufferSize - mBufferPos, dataSize);
            memcpy(data, mBuffer + mBufferPos, avail);
            data += avail;
            dataSize -= avail;
            result += static_cast<int>(avail);
            mBufferPos += avail;
            if (mBufferPos == mBufferSize) {
                // Full reply sent, now wait for the guest 'start' command.
                setExpectedGuestCommand("start",
                                        State::WaitingForGuestStartCommand);
                return result;
            }
        }
        buffers++;
        numBuffers--;
    }
    return result;
}

int AdbGuestPipe::onGuestSendCommand(const AndroidPipeBuffer* buffers,
                                     int numBuffers) {
    DD("%s: [%p] numBuffers=%d", __func__, this, numBuffers);
    CHECK(mState == State::WaitingForGuestAcceptCommand ||
          mState == State::WaitingForGuestStartCommand);
    // Waiting for a command from the guest. Just match the bytes
    // with the expected command. Any mismatch triggers an I/O error
    // which will force the guest to close the connection.
    int result = 0;
    while (numBuffers > 0) {
        const char* data = reinterpret_cast<const char*>(buffers[0].data);
        size_t dataSize = buffers[0].size;
        while (dataSize > 0) {
            size_t avail = std::min(mBufferSize - mBufferPos, dataSize);
            if (memcmp(data, mBuffer + mBufferPos, avail) != 0) {
                // Mismatched, this is not what the pipe is expecting.
                // Closing the connection now is easier than sending 'ko'.
                DD("%s: [%p] mismatched command", __func__, this);
                mHostSocket.reset();
                return PIPE_ERROR_IO;
            }
            data += avail;
            dataSize -= avail;
            result += static_cast<int>(avail);
            mBufferPos += avail;
            if (mBufferPos == mBufferSize) {
                // Expected command was received.
                DD("%s: [%p] command '%.*s' validated", __func__, this,
                   (int)mBufferSize, mBuffer);
                if (mState == State::WaitingForGuestAcceptCommand) {
                    waitForHostConnection();
                } else if (mState == State::WaitingForGuestStartCommand) {
                    // Proxying data can start right now.
                    mState = State::ProxyingData;
                    // when -verbose, print a message indicating adb is connected
                    DINIT("%s: [%p] Adb connected, start proxing data",__func__, this);
                }
                return result;
            }
        }
        numBuffers--;
        buffers++;
    }
    DD("%s: [%p] returning %d with buffer pos/size %d/%d", __func__, this,
       result, (int)mBufferPos, (int)mBufferSize);
    return result;
}

void AdbGuestPipe::setReply(StringView reply, State newState) {
    CHECK(newState == State::SendingAcceptReplyOk);
    CHECK(reply.size() <= sizeof(mBuffer));
    memcpy(mBuffer, reply.c_str(), reply.size());
    mBufferSize = reply.size();
    mBufferPos = 0;
    mState = newState;
}

void AdbGuestPipe::setExpectedGuestCommand(StringView command, State newState) {
    CHECK(newState == State::WaitingForGuestAcceptCommand ||
          newState == State::WaitingForGuestStartCommand);
    CHECK(command.size() <= sizeof(mBuffer));
    memcpy(mBuffer, command.c_str(), command.size());
    mBufferSize = command.size();
    mBufferPos = 0;
    mState = newState;
}

void AdbGuestPipe::waitForHostConnection() {
    // No host connection yet. The guest is still waiting for the 'ok'
    // reply.
    DD("%s: [%p] waiting for host connection", __func__, this);
    mState = State::WaitingForHostAdbConnection;
    // Tell the agent to start listening again.
    // Also notify the server that an emulator instance is waiting
    // for a connection. This is useful when the ADB Server was
    // restarted on the host, but could not see the current emulator
    // instance because it's listening on a 'non-standard' ADB port.
    mHostAgent->startListening();
    mHostAgent->notifyServer();
}

}  // namespace emulation
}  // namespace android
