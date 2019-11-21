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

#include "android/wear-agent/WearAgent.h"

#include "android/base/Log.h"
#include "android/base/async/AsyncReader.h"
#include "android/base/async/AsyncWriter.h"
#include "android/base/async/Looper.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/system/System.h"
#include "android/base/threads/FunctorThread.h"
#include "android/utils/debug.h"
#include "android/wear-agent/PairUpWearPhone.h"

#include <string>
#include <vector>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DPRINT(...)  ((void)0)

//#define  DPRINT(...)  do {  if (VERBOSE_CHECK(adb)) dprintn(__VA_ARGS__); } while (0)

namespace android {
namespace wear {

using namespace ::android::base;

/*
 * Wear Agent listens to adb host server for device list update.
 * Whenever it receives an update, it will abort current pairing process
 * and start a new one. When adb host dies, Wear Agent will try to reconnect
 * every two seconds.
 */

class WearAgentImpl {
public:
    WearAgentImpl(Looper* looper, int adbHostPort);
    ~WearAgentImpl();

    bool connected() const;

    void onWrite();
    void onRead();
    void connectToAdbHostAsync();
    void completeConnect();

private:
    void connectToAdbHostWorker();
    void connectToAdbHost();
    void cleanupConnection();
    void connectLater();
    void completeConnectLater();

    static const int DEFAULT_READ_BUFFER_SIZE = 1024;
    static const int WRITE_BUFFER_SIZE = 1024;
    int          mAdbHostPort;

    enum class SocketOpenState {
        Pending,
        Succeded,
        Failed
    };
    SocketOpenState mSocketOpenState = SocketOpenState::Pending;
    int          mSocket;
    android::base::Lock mSocketLock;

    Looper* mLooper;
    ScopedPtr<Looper::FdWatch> mFdWatch;
    ScopedPtr<Looper::Timer> mTimer;
    int          mRetryAfterSeconds;
    ::android::base::AsyncReader  mAsyncReader;
    ::android::base::AsyncWriter  mAsyncWriter;
    int          mSizeOfReadBuffer;
    char         *mReadBuffer;
    char         mWriteBuffer[WRITE_BUFFER_SIZE];
    int          mExpectReplayType;
    ScopedPtr<PairUpWearPhone> mPairUpWearPhone;

    enum class ConnectAction {
        None,
        Connect,
        Quit
    };

    ConnectAction mConnectAction = ConnectAction::None;
    android::base::FunctorThread mConnectThread;
    android::base::ConditionVariable mConnectCondition;
    android::base::Lock mConnectLock;

    // Any looper interaction is safe only on the looper's own
    // thread. This timer will call the completeConnect()
    // function on the main thread where it's safe
    ScopedPtr<Looper::Timer> mConnectCompleteTimer;

    enum ExpectMessage {
        OKAY = 0,
        LENGTH,
        MESSAGE
    };

    bool expectOkay () const { return OKAY == mExpectReplayType; }
    bool expectLength() const { return LENGTH == mExpectReplayType; }
    bool expectMsg() const { return MESSAGE == mExpectReplayType; }
    static bool isValidHexNumber(const char* str, const int sz);
    static void parseAdbDevices(char* buf, std::vector<std::string>* devices);

    DISALLOW_COPY_ASSIGN_AND_MOVE(WearAgentImpl);
};

// This callback is called whenever an I/O event happens on the socket
// connecting the agent to the host ADB server.
static void on_adb_server_socket_fd(void* opaque, int fd, unsigned events) {
    const auto agent = static_cast<WearAgentImpl*>(opaque);
    if (!agent) {
        return;
    }
    if ((events & Looper::FdWatch::kEventRead) != 0) {
        agent->onRead();
    } else if ((events & Looper::FdWatch::kEventWrite) != 0) {
        agent->onWrite();
    }
}

bool WearAgentImpl::connected() const
{
    return mFdWatch.get() != nullptr;
}

void WearAgentImpl::onRead() {
    if (!connected()) {
        return;
    }

    bool isError = false;
    int msgsize = 0;
    AsyncStatus status = mAsyncReader.run();

    switch (status) {
        case kAsyncCompleted:
            if (expectOkay() && !strncmp("OKAY", mReadBuffer, 4)) {
                mTimer->stop();
                mReadBuffer[4] = '\0';
                mAsyncReader.reset(mReadBuffer, 4, mFdWatch.get());
                mExpectReplayType = LENGTH;

            } else if (expectLength() && isValidHexNumber(mReadBuffer, 4)
                    && 1 == sscanf(mReadBuffer, "%x", &msgsize)) {
                if (msgsize < 0) {
                    isError = true;
                } else if (0 == msgsize) { //this is not error: just no devices
                    mExpectReplayType = LENGTH;
                    mReadBuffer[msgsize] = '\0';
                    mAsyncReader.reset(mReadBuffer, 4, mFdWatch.get());
                } else {
                    if (msgsize >= mSizeOfReadBuffer) {
                        char* ptr = (char*)calloc(2 * msgsize, sizeof(char));
                        if (ptr) {
                            mSizeOfReadBuffer = 2 * msgsize;
                            free(mReadBuffer);
                            mReadBuffer = ptr;
                        }
                    }
                    mExpectReplayType = MESSAGE;
                    mReadBuffer[msgsize] = '\0';
                    mAsyncReader.reset(mReadBuffer, msgsize, mFdWatch.get());
                }

            } else if (expectMsg()) {
                DPRINT("message received from ADB:\n%s", mReadBuffer);
                mPairUpWearPhone.reset();

                std::vector<std::string> devices;
                parseAdbDevices(mReadBuffer, &devices);
                if (devices.size() >= 2) {
                    mPairUpWearPhone.reset(new PairUpWearPhone(mLooper,
                                                               devices,
                                                               mAdbHostPort));
                }
                // prepare for next adb devices message
                mReadBuffer[4] = '\0';
                mAsyncReader.reset(mReadBuffer, 4, mFdWatch.get());
                mExpectReplayType = LENGTH;
            } else {
                isError = true;
            }
            break;

        case kAsyncAgain:
            return;

        case kAsyncError:
            isError = true;
            break;
    }

    if (isError) {
        cleanupConnection();
        connectLater();
    }
}

void WearAgentImpl::onWrite() {
    if (!connected()) {
        return;
    }

    AsyncStatus status = mAsyncWriter.run();
    switch (status) {
        case kAsyncCompleted:
            mReadBuffer[4] = '\0';
            mAsyncReader.reset(mReadBuffer, 4, mFdWatch.get());
            mExpectReplayType = OKAY;
            return;
        case kAsyncError:
            cleanupConnection();
            connectLater();
            return;
        case kAsyncAgain:
            return;
    }
}

void WearAgentImpl::connectToAdbHostAsync() {
    if (connected()) {
        return;
    }

    {
        android::base::AutoLock lock(mSocketLock);
        mSocketOpenState = SocketOpenState::Pending;
    }
    completeConnectLater();

    {
        android::base::AutoLock lock(mConnectLock);
        mConnectAction = ConnectAction::Connect;
        mConnectCondition.signal();
    }
}

void WearAgentImpl::connectToAdbHostWorker()
{
    Thread::maskAllSignals();

    for (;;) {
        android::base::AutoLock lock(mConnectLock);
        while (mConnectAction == ConnectAction::None) {
            mConnectCondition.wait(&mConnectLock);
        }

        switch (mConnectAction) {
        default:
            break;  // make the compiler happy

        case ConnectAction::Quit:
            return;

        case ConnectAction::Connect:
            mConnectAction = ConnectAction::None;
            lock.unlock();
            connectToAdbHost();
            break;
        }
    }
}

void WearAgentImpl::connectToAdbHost() {
    // this is the only function which modifies members from a separate thread
    // it means we need to be extra careful changing it:
    // - make sure correct locks are locked
    // - make sure the state is updated only after everything else
    // - don't hold locks for too long

    const int socket = socketTcp4LoopbackClient(mAdbHostPort);
    if (socket < 0) {
        android::base::AutoLock lock(mSocketLock);
        mSocketOpenState = SocketOpenState::Failed;
        return;
    }

    socketSetNonBlocking(socket);

    android::base::AutoLock lock(mSocketLock);
    if (mSocket >= 0) {
        // already created, make sure the completion runs
        mSocketOpenState = SocketOpenState::Succeded;
        lock.unlock();
        socketClose(socket);
        return;
    }

    mSocket = socket;
    mSocketOpenState = SocketOpenState::Succeded;
}

void WearAgentImpl::connectLater() {
    DPRINT("Warning: cannot connect to adb server, will try again in %d seconds. \n",
            mRetryAfterSeconds);
    const Looper::Duration dl = 1000 * mRetryAfterSeconds;
    mTimer->startRelative(dl);
}

void WearAgentImpl::completeConnectLater()
{
    static const unsigned completeConnectTimeoutMs = 16;

    DPRINT("Info: scheduling a connection completion handler in %d ms. \n",
            completeConnectTimeoutMs);
    mConnectCompleteTimer->startRelative(completeConnectTimeoutMs);
}

void WearAgentImpl::completeConnect()
{
    if (connected()) {
        return;
    }

    android::base::AutoLock lock(mSocketLock);
    const auto state = mSocketOpenState;
    mSocketOpenState = SocketOpenState::Pending;
    lock.unlock();

    switch (state) {
    case SocketOpenState::Pending:
        // just reschedule to check later
        completeConnectLater();
        return;

    case SocketOpenState::Failed:
        connectLater();
        return;

    case SocketOpenState::Succeded:
        {
            assert(mSocket >= 0);

            ScopedPtr<Looper::FdWatch> fdWatch(mLooper->createFdWatch(
                                                   mSocket,
                                                   on_adb_server_socket_fd,
                                                   this));
            mAsyncWriter.reset(mWriteBuffer,
                               ::strlen(mWriteBuffer),
                               fdWatch.get());

            // connected() checks if mFdWatch is not null, so set it last
            mFdWatch.reset(fdWatch.release());
        }
        break;
    }
}

void WearAgentImpl::cleanupConnection() {
    mFdWatch.reset();

    {
        android::base::AutoLock lock(mSocketLock);
        if (mSocket >= 0) {
            socketClose(mSocket);
            mSocket = -1;
        }
    }

    mPairUpWearPhone.reset();
}

void WearAgentImpl::parseAdbDevices(char* buf,
                                    std::vector<std::string>* devices) {
    if (!buf) return;

    static const char kDelimiters[] = " \t\r\n";

    char* pch = strtok(buf, kDelimiters);
    char* prev = NULL;
    while (pch) {
        if (!strcmp("device", pch) && prev) {
            devices->push_back(std::string(prev));
        }
        prev = pch;
        pch = strtok(NULL, kDelimiters);
    }
}

bool WearAgentImpl::isValidHexNumber(const char* str, const int sz) {
    if (!str || sz <= 0)
        return false;
    for (int i=0; i < sz; ++i) {
        int ch = tolower(str[i]);
        if ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f')) {
            continue;
        } else {
            return false;
        }
    }
    return true;
}

static void on_connect_complete(void* opaque, Looper::Timer* timer) {
    const auto agent = static_cast<WearAgentImpl*>(opaque);
    if (agent) {
        agent->completeConnect();
    }
}

static void on_reconnect_timeout(void* opaque, Looper::Timer* timer) {
    const auto agent = static_cast<WearAgentImpl*>(opaque);
    if (agent) {
        agent->connectToAdbHostAsync();
    }
}

WearAgentImpl::WearAgentImpl(Looper* looper, int adbHostPort) :
        mAdbHostPort(adbHostPort),
        mSocket(-1),
        mLooper(looper),
        mFdWatch(),
        mTimer(),
        mRetryAfterSeconds(2),
        mAsyncReader(),
        mAsyncWriter(),
        mSizeOfReadBuffer(DEFAULT_READ_BUFFER_SIZE),
        mReadBuffer(0),
        mWriteBuffer(),
        mExpectReplayType(OKAY),
        mPairUpWearPhone(),
        mConnectThread([this]() { connectToAdbHostWorker(); return 0; })
{
    mReadBuffer = (char*)calloc(mSizeOfReadBuffer,sizeof(char));
    mTimer.reset(looper->createTimer(on_reconnect_timeout, this));

    static const char kTrackDevicesRequest[] = "host:track-devices";
    snprintf(mWriteBuffer, sizeof(mWriteBuffer), "%04x%s",
             static_cast<unsigned>(sizeof(kTrackDevicesRequest) - 1U),
             kTrackDevicesRequest);

    mConnectCompleteTimer.reset(looper->createTimer(on_connect_complete, this));

    mConnectThread.start();
    connectToAdbHostAsync();
}

WearAgentImpl::~WearAgentImpl() {
    {
        android::base::AutoLock lock(mConnectLock);
        mConnectAction = ConnectAction::Quit;
        mConnectCondition.signal();
    }
    mConnectThread.wait();

    cleanupConnection();
    free(mReadBuffer);
}

//------------------------- WearAgent Class
WearAgent::WearAgent(Looper* looper, int adbHostPort) : mWearAgentImpl(0) {
    if (looper && adbHostPort >= 5037) {
        mWearAgentImpl = new WearAgentImpl(looper, adbHostPort);
    }
}

WearAgent::~WearAgent() {
    if (mWearAgentImpl) {
        delete mWearAgentImpl;
        mWearAgentImpl = 0;
    }
}

} // namespace wear
} // namespace android
