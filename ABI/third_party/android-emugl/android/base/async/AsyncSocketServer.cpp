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

#include "android/base/async/AsyncSocketServer.h"

#include "android/base/async/Looper.h"
#include "android/base/async/ScopedSocketWatch.h"
#include "android/base/async/ThreadLooper.h"
#include "android/base/Log.h"
#include "android/base/sockets/ScopedSocket.h"
#include "android/base/sockets/SocketUtils.h"

#include <memory>

namespace android {
namespace base {

using FdWatch = android::base::Looper::FdWatch;

namespace {

class BaseSocketServer : public AsyncSocketServer {
public:
    BaseSocketServer(Looper* looper,
                     int port,
                     ConnectCallback connectCallback,
                     int socket4,
                     int socket6)
        : mLooper(looper), mPort(port), mConnectCallback(connectCallback) {
        if (socket4 >= 0) {
            mBoundSocket4.reset(looper->createFdWatch(socket4, onAccept, this));
            CHECK(mBoundSocket4.get());
        }
        if (socket6 >= 0) {
            mBoundSocket6.reset(looper->createFdWatch(socket6, onAccept, this));
            CHECK(mBoundSocket6.get());
        }
    }

    virtual ~BaseSocketServer() = default;

    virtual int port() const override { return mPort; }

    virtual void startListening() override {
        if (mBoundSocket4) {
            mBoundSocket4->wantRead();
        }
        if (mBoundSocket6) {
            mBoundSocket6->wantRead();
        }
        mListening = true;
    };

    bool isListening() const {
        return mListening;
    };

    virtual void stopListening() override {
        if (mBoundSocket4) {
            mBoundSocket4->dontWantRead();
        }
        if (mBoundSocket6) {
            mBoundSocket6->dontWantRead();
        }
        mListening = false;
    };

    virtual LoopbackMode getListenMode() const override {
        return mBoundSocket4 ? (mBoundSocket6 ? kIPv4AndIPv6 : kIPv4)
                             : (mBoundSocket6 ? kIPv6 : kNone);
    }

private:
    // Called when an i/o event happens on the bound socket. Typically
    // a read event indicates a new client connection.
    static void onAccept(void* opaque, int fd, unsigned events) {
        auto server = reinterpret_cast<BaseSocketServer*>(opaque);
        if (!server->isListening()) {
            // If we listen on IPv4 and IPv6 both, then we could get onAccept from one socket
            // even we've already called stopListening on another socket if we get connections
            // from both sockets around same time. In such case, just ignore onAccept.
            CHECK(server->getListenMode() == kIPv4AndIPv6) << "Hit onAccept after stopListening";
            return;
        }
        if (events & FdWatch::kEventRead) {
            int clientFd = socketAcceptAny(fd);
            if (clientFd < 0) {
                // This can happen when the client closed the connection
                // before we could process it. Just exit and try until the
                // next time.
                PLOG(WARNING) << "Error when accepting host connection";
                return;
            }
            // Stop listening now, this lets the callback call startListening()
            // if it wants to allow new connections immediately. It also means
            // we need to call it explicitly in case of error.
            server->stopListening();
            if (!server->mConnectCallback(clientFd)) {
                // Assume the callback printed any necessary error message.
                socketClose(clientFd);
                server->startListening();
                return;
            }
        }
    }

    ScopedSocketWatch mBoundSocket4;
    ScopedSocketWatch mBoundSocket6;
    Looper* mLooper = nullptr;
    int mPort = 0;
    bool mListening = false;
    ConnectCallback mConnectCallback;
};

}  // namespace

// static
std::unique_ptr<AsyncSocketServer> AsyncSocketServer::createTcpLoopbackServer(
        int port,
        ConnectCallback connectCallback,
        LoopbackMode mode,
        Looper* looper) {
    if (!looper) {
        looper = ThreadLooper::get();
    }
    ScopedSocket sock4;
    ScopedSocket sock6;

    // If |port| is 0, we let the system find a random free port, but
    // in the case of kIPv4AndIPv6, we need to ensure that it is possible
    // to bind to both ports. Loop a few times to ensure that this is the
    // case.
    int tries = (port == 0 && mode == kIPv4AndIPv6) ? 5 : 1;
    int port0 = port;
    bool success = false;
    for (; tries > 0; tries--) {
        port = port0;
        success = true;
        if ((mode & kIPv4) != 0) {
            sock4.reset(socketTcp4LoopbackServer(port));
            if (!sock4.valid()) {
                if (!(mode & kIPv4Optional)) {
                    success = false;
                }
            } else if (!port) {
                port = socketGetPort(sock4.get());
            }
        }
        if ((mode & kIPv6) != 0) {
            sock6.reset(socketTcp6LoopbackServer(port));
            if (!sock6.valid()) {
                if (!(mode & kIPv6Optional)) {
                    success = false;
                }
            } else if (!port) {
                port = socketGetPort(sock6.get());
            }
        }

        if (success) { break; }
    }

    AsyncSocketServer* result = nullptr;

    if (success && (sock4.valid() || sock6.valid())) {
        result = new BaseSocketServer(looper, port, connectCallback,
                                      sock4.release(), sock6.release());
    }
    return std::unique_ptr<AsyncSocketServer>(result);
}

}  // namespace base
}  // namespace android
