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

#include "android/base/sockets/SocketDrainer.h"

#include "android/base/EintrWrapper.h"
#include "android/base/sockets/SocketErrors.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/base/async/Looper.h"

#include <unordered_set>

// Some implementation whys:
// When the looper is running, the sockets are non-blocking and are only
// closed after all data has been read.
// When looper stops, the remaining sockets are still non-blocking and are
// closed after all available data has been read (cannot set to blocking, because
// doing that could put the program sleeping forever).
// When socket is already closed for read, we only need to shutdown writing
// and close the socket.

namespace android {
namespace base {

class DrainerObject;

// SocketDrainImpl implements the SocketDrainer and manages all the DrainerObjects
class SocketDrainerImpl {
    DISALLOW_COPY_AND_ASSIGN(SocketDrainerImpl);

public:
    SocketDrainerImpl(Looper* looper) : mLooper(looper) {}
    ~SocketDrainerImpl();

public:
    void addSocketToDrain(int socket_fd);
    void removeDrainerObject(DrainerObject* drainer);

private:
    using DrainSet = std::unordered_set<DrainerObject*>;

    Looper*  mLooper;
    DrainSet mDrainerObjects;
};

// DrainerObject drains and closes socket
class DrainerObject {
public:
    DrainerObject(int socket_fd, Looper* looper, SocketDrainerImpl* parent);
    ~DrainerObject();
public:
    // drain socket and return true if there is still more data to drain
    bool drainSocket();

    // return true when there is no data to drain
    bool socketIsDrained() const { return mSocketIsDrained; }

    // remove from SocketDrainerImpl
    void removeFromParent() {
        if(mParent) {
            mParent->removeDrainerObject(this);
        }
    }

private:
    int     mSocket;
    Looper* mLooper;
    SocketDrainerImpl* mParent;
    Looper::FdWatch*  mIo;
    bool    mSocketIsDrained;

    void shutdownRead();
    void shutdownWrite();
    void closeSocket();
};

// callback from looper when the socket_fd has some data ready to read
static void _on_read_socket_fd(void* opaque, int fd, unsigned events) {
    DrainerObject * drainerObject = (DrainerObject*)opaque;
    if (!drainerObject) return;
    if ((events & Looper::FdWatch::kEventRead) != 0) {
        drainerObject->drainSocket();
    }
    if (drainerObject->socketIsDrained()) {
        drainerObject->removeFromParent();
    }
}

DrainerObject::DrainerObject(int socket_fd,
                             Looper* looper,
                             SocketDrainerImpl* parent) :
            mSocket(socket_fd),
            mLooper(looper),
            mParent(parent),
            mIo(NULL),
            mSocketIsDrained(false) {
    socketShutdownWrites(mSocket);
    if (drainSocket() && mLooper && mParent) {
        mIo = looper->createFdWatch(mSocket, _on_read_socket_fd, this);
        mIo->wantRead();
        mIo->dontWantWrite();
    } else {
        // there is nothing to read, the drainer object is done
        mLooper = 0;
    }
}

DrainerObject::~DrainerObject() {
    if (!mSocketIsDrained) {
        char buff[1024];
        while(socketRecv(mSocket, buff, sizeof(buff)) > 0) {}
        mSocketIsDrained = true;
    }
    socketShutdownReads(mSocket);
    delete mIo;
    socketClose(mSocket);
    mSocket = -1;
    mParent = 0;
}

bool DrainerObject::drainSocket() {
    errno = 0;
    char buff[1024];
    ssize_t size = socketRecv(mSocket, buff, sizeof(buff));
    if (size > 0) {
        return true;
    } else if (size < 0 && errno == EWOULDBLOCK) {
        return true;
    }
    mSocketIsDrained = true;
    return false;
}

//--------------------------- SocketDrainerImpl Implementation -------------------------

SocketDrainerImpl::~SocketDrainerImpl() {
    for (auto drainer : mDrainerObjects) {
        delete drainer;
    }
}

void SocketDrainerImpl::addSocketToDrain(int socket_fd) {
    DrainerObject* drainer = new DrainerObject(socket_fd, mLooper, this);
    if (drainer->socketIsDrained()) {
        delete drainer;
    } else {
        mDrainerObjects.insert(drainer);
    }
}

void SocketDrainerImpl::removeDrainerObject(DrainerObject* drainer) {
    mDrainerObjects.erase(drainer);
    delete drainer;
}

//--------------------------- SocketDrainer Implementation -----------------------------

SocketDrainer::SocketDrainer(Looper* looper) :
    mSocketDrainerImpl(new SocketDrainerImpl(looper)) {
}

SocketDrainer::~SocketDrainer() {
    delete mSocketDrainerImpl;
    mSocketDrainerImpl = 0;
}

void SocketDrainer::drainAndClose(int socketFd) {
    if (socketFd < 0) {
        return;
    }
    mSocketDrainerImpl->addSocketToDrain(socketFd);
}

// static
void SocketDrainer::drainAndCloseBlocking(int socketFd) {
    DrainerObject drainer(socketFd, 0, 0);
}

} // namespace base
} // namespace android

// -------------------- extern C functions ---------------------------------------------

