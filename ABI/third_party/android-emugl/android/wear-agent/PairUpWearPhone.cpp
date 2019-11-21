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

#include "android/wear-agent/PairUpWearPhone.h"


#include "android/base/async/AsyncReader.h"
#include "android/base/async/AsyncStatus.h"
#include "android/base/async/AsyncWriter.h"
#include "android/base/async/Looper.h"
#include "android/base/Log.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/base/misc/StringUtils.h"
#include "android/base/sockets/SocketErrors.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/utils/debug.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define  DPRINT(...)  do {  if (VERBOSE_CHECK(adb)) dprintn(__VA_ARGS__); } while (0)

namespace android {
namespace wear {

using namespace ::android::base;

typedef ScopedPtr<Looper::FdWatch> FdWatch;
typedef ScopedPtr<Looper::Timer> Timer;

class PairUpWearPhoneImpl {
public:
    PairUpWearPhoneImpl(Looper* looper,
                        const std::vector<std::string>& devices,
                        int adbHostPort);

    ~PairUpWearPhoneImpl();

    void writeAdbServerSocket();
    void readAdbServerSocket();

    void writeEmulatorConsoleSocket();
    void readEmulatorConsoleSocket();

    bool isDone() const;
    void cleanUp();

private:
    typedef std::vector<std::string> StrQueue;
    enum CommunicationState {

         PAIRUP_ERROR = 0,

         // when ask adb for device property, need to first ask adb to setup transfer,
         // wait for "OKAY" from adb, then send the query command(which is transferred
         // by adb to the device), and read reply from adb until adb disconnect

         // get product name to distinguish wear from phone
         TO_GET_PRODUCT_NAME_INIT_XFER,
         TO_GET_PRODUCT_NAME_WAIT_OKAY,
         TO_GET_PRODUCT_NAME_SEND_CMD,
         TO_GET_PRODUCT_NAME_READ_REPLY,

         // find out if the phone has wearable app, which is responsible for communication
         // with wear
         TO_GET_WEARABLE_APP_INIT_XFER,
         TO_GET_WEARABLE_APP_WAIT_OKAY,
         TO_GET_WEARABLE_APP_SEND_CMD,
         TO_GET_WEARABLE_APP_READ_REPLY,

         // perform ip forwarding on real device
         // "host-serial:phone-serial-number:forward:tcp:5601;5601"
         TO_FORWARD_IP_SEND_CMD,
         TO_FORWARD_IP_WAIT_OKAY,

         // perform ip redirection on emulator
         // the above command does not work on emulator, so we have to connect to the emulator
         // console and issue command "redir add"
         TO_READ_GREETINGS_FROM_CONSOLE,
         TO_SEND_REDIR_CMD_TO_CONSOLE,

         PAIRUP_SUCCESS,

         PAIRUP_ALREADY_CLEANEDUP
    };
    enum MessageType {
         TRANSFER = 1,
         FORWARDIP,
         PROPERTY,
         WEARABLE_APP_PKG
    };
    static const int READ_BUFFER_SIZE  = 1024;
    static const int WRITE_BUFFER_SIZE = 1024;

    Looper* mLooper;

    // member related to adb host
    int mAdbHostPort;
    FdWatch mAdbWatch;

    // member related to emulator console
    int mConsolePort;
    FdWatch mConsoleWatch;

    // device categories
    std::string mDeviceInProbing;
    StrQueue mUnprobedDevices;
    StrQueue mWearDevices;
    StrQueue mPhoneDevices;

    CommunicationState mState;

    // asynchronous reader and writer
    AsyncReader mAsyncReader;
    AsyncWriter mAsyncWriter;
    char* mReadBuffer;
    char* mWriteBuffer;
    std::string mReply;

    // whenever device list changes, call the following
    void updateDevices(const std::vector<std::string>& devices);
    void startProbeNextDevice();

    // socket connection methods
    bool openConnection(int port,
                        FdWatch* watch,
                        Looper::FdWatch::Callback callback);

    void closeConnection(FdWatch* watch);
    void openAdbConnection();
    void openConsoleConnection();

    // read and write mode handling methods
    void switchToRead(FdWatch* watch) {
        (*watch)->wantRead();
        (*watch)->dontWantWrite();
    }

    void switchToWrite(FdWatch* watch) {
        (*watch)->wantWrite();
        (*watch)->dontWantRead();
    }

    // connect wear to phone by setuping port forwarding on emulator or real device
    bool checkForWearDevice();
    bool checkForCompatiblePhone();
    void startConnectWearAndPhone();

    bool startReadConsole();

    // adb asynchronous read/write methods: all return true on success
    // false if need to try again or on error. When error happens,
    // will call startProbeNextDevice
    bool startWriteCommandToAdb(int queryType, const char* message);
    bool completeWriteCommandToAdb();
    bool startReadHeaderFromAdb();
    bool completeReadHeaderFromAdb();
    bool completeReadAllDataFromAdb();
};

static const char kWearableAppName[] = "com.google.android.wearable";

// callback that talks to adb server on the host computer
static void _on_adb_server_socket_fd(void* opaque, int fd, unsigned events) {
    PairUpWearPhoneImpl * pairup = static_cast<PairUpWearPhoneImpl*>(opaque);
    if (!pairup) return;

    if (events & Looper::FdWatch::kEventRead) {
        pairup->readAdbServerSocket();
    } else if (events & Looper::FdWatch::kEventWrite) {
        pairup->writeAdbServerSocket();
    }
    if (pairup->isDone()) {
        pairup->cleanUp();
    }
}

// callback that talks to emulator console
static void _on_emulator_console_socket_fd(void* opaque, int fd, unsigned events) {
    PairUpWearPhoneImpl * pairup = static_cast<PairUpWearPhoneImpl*>(opaque);
    if (!pairup) return;

    if (events & Looper::FdWatch::kEventRead) {
        pairup->readEmulatorConsoleSocket();
    } else if (events & Looper::FdWatch::kEventWrite) {
        pairup->writeEmulatorConsoleSocket();
    }
    if (pairup->isDone()) {
        pairup->cleanUp();
    }
}

//-------------- callbacks for IO with adb server and emulator console
void PairUpWearPhoneImpl::readAdbServerSocket() {

    switch(mState) {
        case TO_GET_PRODUCT_NAME_WAIT_OKAY:
            if (!completeReadHeaderFromAdb()) return;
            if (!startWriteCommandToAdb(PROPERTY, "ro.product.name")) return;
            mState = TO_GET_PRODUCT_NAME_SEND_CMD;
            return;

        case TO_GET_PRODUCT_NAME_READ_REPLY:
            if (!completeReadAllDataFromAdb()) return;
            if (checkForWearDevice()) {
                startConnectWearAndPhone();
                return;
            }
            if (!startWriteCommandToAdb(TRANSFER, mDeviceInProbing.c_str())) return;
            mState = TO_GET_WEARABLE_APP_INIT_XFER;
            return;

        case TO_GET_WEARABLE_APP_WAIT_OKAY:
            if (!completeReadHeaderFromAdb()) return;
            if (!startWriteCommandToAdb(WEARABLE_APP_PKG, kWearableAppName)) return;
            mState = TO_GET_WEARABLE_APP_SEND_CMD;
            return;

        case TO_GET_WEARABLE_APP_READ_REPLY:
            if (!completeReadAllDataFromAdb()) return;
            if (checkForCompatiblePhone()) {
                startConnectWearAndPhone();
                return;
            }
            startProbeNextDevice();
            return;

        case TO_FORWARD_IP_WAIT_OKAY:
            if (!completeReadHeaderFromAdb()) return;
            mState = PAIRUP_SUCCESS;
            return;

        default:
            startProbeNextDevice();
            return;
    }
}

void PairUpWearPhoneImpl::writeAdbServerSocket() {

    switch (mState) {
        case TO_GET_PRODUCT_NAME_INIT_XFER:
            if (!completeWriteCommandToAdb()) return;
            if (!startReadHeaderFromAdb()) return;
            mState = TO_GET_PRODUCT_NAME_WAIT_OKAY;
            return;

        case TO_GET_PRODUCT_NAME_SEND_CMD:
            if (!completeWriteCommandToAdb()) return;
            mState = TO_GET_PRODUCT_NAME_READ_REPLY;
            mReply.clear();
            switchToRead(&mAdbWatch);
            return;

        case TO_GET_WEARABLE_APP_INIT_XFER:
            if (!completeWriteCommandToAdb()) return;
            if (!startReadHeaderFromAdb()) return;
            mState = TO_GET_WEARABLE_APP_WAIT_OKAY;
            return;

        case TO_GET_WEARABLE_APP_SEND_CMD:
            if (!completeWriteCommandToAdb()) return;
            mState = TO_GET_WEARABLE_APP_READ_REPLY;
            mReply.clear();
            switchToRead(&mAdbWatch);
            return;

        case TO_FORWARD_IP_SEND_CMD:
            if (!completeWriteCommandToAdb()) return;
            if (!startReadHeaderFromAdb()) return;
            mState = TO_FORWARD_IP_WAIT_OKAY;
            return;

        default:
            startProbeNextDevice();
            return;
    }

}

void PairUpWearPhoneImpl::readEmulatorConsoleSocket() {
    const ssize_t size = socketRecv(mConsoleWatch->fd(),
                                    mReadBuffer,
                                    READ_BUFFER_SIZE);
    if (size > 0) {
        mReply.append(mReadBuffer, size);
        DPRINT("read console message: %s\n", mReply.c_str());
        if (strContains(mReply, "OK")) {
            DPRINT("received mesg from console:\n%s\n", mReply.c_str());
            snprintf(mWriteBuffer, WRITE_BUFFER_SIZE, "redir add tcp:5601:5601\nquit\n");
            DPRINT("sending query to console:\n%s", mWriteBuffer);
            mAsyncWriter.reset(mWriteBuffer,
                               ::strlen(mWriteBuffer),
                               mConsoleWatch.get());
            switchToWrite(&mConsoleWatch);
            mState = TO_SEND_REDIR_CMD_TO_CONSOLE;
        }
    } else {
        DPRINT("error happened when reading console\n");
        startProbeNextDevice();
    }
}

void PairUpWearPhoneImpl::writeEmulatorConsoleSocket() {
    const AsyncStatus status = mAsyncWriter.run();

    if (status == kAsyncCompleted) {
        mState = PAIRUP_SUCCESS;
    } else if (status == kAsyncError) {
        DPRINT("Error: cannot write to console\n");
        startProbeNextDevice();
    }
}

// -------------------------------   Other helper methods
bool PairUpWearPhoneImpl::checkForWearDevice() {
    const bool isWearDevice =
            strContains(mReply, "clockwork") &&
            !strncmp(mDeviceInProbing.c_str(), "emulator-", 9);

    if (isWearDevice) {
        DPRINT("found wear %s\n\n", mDeviceInProbing.c_str());
        mWearDevices.push_back(mDeviceInProbing);
    }

    return isWearDevice;
}

bool PairUpWearPhoneImpl::checkForCompatiblePhone() {
    const bool isCompatiblePhone = strContains(mReply, kWearableAppName);

    if (isCompatiblePhone) {
        DPRINT("found compatible phone %s\n\n", mDeviceInProbing.c_str());
        mPhoneDevices.push_back(mDeviceInProbing);
    } else {
        const bool shouldTryAgainLater =
                strContains(mReply, "Error:") &&
                strContains(mReply, "Is the system running?");

        if (shouldTryAgainLater) mUnprobedDevices.push_back(mDeviceInProbing);
    }

    return isCompatiblePhone;
}

bool PairUpWearPhoneImpl::startReadConsole() {
    openConsoleConnection();

    if (mConsoleWatch->fd() < 0) {
        startProbeNextDevice();
        return false;
    } else {
        DPRINT("ready to read console greetings\n");
        switchToRead(&mConsoleWatch);
        return true;
    }
}

bool PairUpWearPhoneImpl::startWriteCommandToAdb(int queryType,
                                                 const char* message) {
    if (TRANSFER == queryType || FORWARDIP == queryType) {
        closeConnection(&mAdbWatch);
        openAdbConnection();
    }

    if (!mAdbWatch.get() || !message) {
        startProbeNextDevice();
        return false;
    }

    char buf2[1024];
    switch(queryType) {
        case TRANSFER:
            snprintf(buf2, sizeof(buf2), "host:transport:%s", message);
            break;
        case PROPERTY:
            snprintf(buf2, sizeof(buf2), "shell:getprop %s", message);
            break;
        case WEARABLE_APP_PKG:
            snprintf(buf2, sizeof(buf2), "shell:pm list packages %s", message);
            break;
        case FORWARDIP:
            snprintf(buf2, sizeof(buf2), "host-serial:%s:forward:tcp:5601;tcp:5601", message);
            break;
        default:
            startProbeNextDevice();
            return false;
    }

    snprintf(mWriteBuffer, WRITE_BUFFER_SIZE, "%04x%s", (unsigned)(::strlen(buf2)), buf2);
    DPRINT("sending query '%s' to adb\n", mWriteBuffer);
    mAsyncWriter.reset(mWriteBuffer, ::strlen(mWriteBuffer), mAdbWatch.get());
    switchToWrite(&mAdbWatch);
    return true;
}

bool PairUpWearPhoneImpl::completeWriteCommandToAdb() {
    AsyncStatus status = mAsyncWriter.run();

    if (status == kAsyncError) {
        startProbeNextDevice();
        return false;
    } else if (status == kAsyncAgain) {
        return false;
    }
    return true;
}

bool PairUpWearPhoneImpl::startReadHeaderFromAdb() {
    if (!mAdbWatch.get()) {
        startProbeNextDevice();
        return false;
    }

    mReply.clear();
    mReadBuffer[4]='\0';
    mAsyncReader.reset(mReadBuffer, 4, mAdbWatch.get());
    switchToRead(&mAdbWatch);
    return true;
}

bool PairUpWearPhoneImpl::completeReadHeaderFromAdb() {
    AsyncStatus status = mAsyncReader.run();

    if (status == kAsyncAgain) {
        return false;
    } else if (status == kAsyncCompleted && !strncmp("OKAY", mReadBuffer,4)) {
        DPRINT("received mesg from adb host:%s\n", mReadBuffer);
        return true;
    } else {
        startProbeNextDevice();
        return false;
    }
}

bool PairUpWearPhoneImpl::completeReadAllDataFromAdb() {
    if (!mAdbWatch.get()) {
        startProbeNextDevice();
        return false;
    }

    char buff[1024];
    ssize_t size = socketRecv(mAdbWatch->fd(), buff, sizeof(buff));
    if (size < 0) {
        if (errno != EWOULDBLOCK && errno != EAGAIN) {
            startProbeNextDevice();
        }
        return false;
    } else if (size > 0) {
        mReply.append(buff, size);
        return false;
    }
    DPRINT("received mesg from adb host:%s\n", mReply.c_str());
    return true;
}

PairUpWearPhoneImpl::PairUpWearPhoneImpl(
        Looper* looper,
        const std::vector<std::string>& devices,
        int adbHostPort)
    : mLooper(looper),
      mAdbHostPort(adbHostPort),
      mAdbWatch(),
      mConsolePort(-1),
      mConsoleWatch(),
      mUnprobedDevices(),
      mWearDevices(),
      mPhoneDevices(),
      mState(PAIRUP_ERROR),
      mAsyncReader(),
      mAsyncWriter(),
      mReadBuffer(static_cast<char*>(calloc(READ_BUFFER_SIZE, 1))),
      mWriteBuffer(static_cast<char*>(calloc(WRITE_BUFFER_SIZE, 1))),
      mReply() {
    updateDevices(devices);

    if (isDone()) {
        cleanUp();
    }
}

PairUpWearPhoneImpl::~PairUpWearPhoneImpl() {
    cleanUp();
    mLooper = 0;
}

void PairUpWearPhoneImpl::startProbeNextDevice() {
    closeConnection(&mAdbWatch);
    closeConnection(&mConsoleWatch);

    if (mUnprobedDevices.empty()) {
        mState = PAIRUP_ERROR;
        return;
    }

    mDeviceInProbing = mUnprobedDevices[0];
    mUnprobedDevices.erase(mUnprobedDevices.begin());

    if (startWriteCommandToAdb(TRANSFER, mDeviceInProbing.c_str())) {
        mState = TO_GET_PRODUCT_NAME_INIT_XFER;
    } else {
        mState = PAIRUP_ERROR;
    }
}

void PairUpWearPhoneImpl::cleanUp() {
    closeConnection(&mAdbWatch);
    closeConnection(&mConsoleWatch);

    mUnprobedDevices.resize(0U);
    mPhoneDevices.resize(0U);
    mWearDevices.resize(0U);
    mReply.clear();
    if (mReadBuffer) {
        free(mReadBuffer);
        mReadBuffer = NULL;
    }
    if (mWriteBuffer) {
        free(mWriteBuffer);
        mWriteBuffer = NULL;
    }
    if (PAIRUP_SUCCESS == mState) {
        DPRINT("\nSUCCESS\n");
    } else if (PAIRUP_ALREADY_CLEANEDUP != mState) {
        DPRINT("\nFAIL\n");
    }
    mState = PAIRUP_ALREADY_CLEANEDUP;
    DPRINT("\nclean up\n\n");
    fflush(stdout); // actually, need to flush stdout
}

bool PairUpWearPhoneImpl::isDone() const {
    return (PAIRUP_ERROR == mState || PAIRUP_SUCCESS == mState);
}

void PairUpWearPhoneImpl::updateDevices(
        const std::vector<std::string>& devices) {
    if (devices.size() < 2) {
        DPRINT("Error: There are less than two devices, cannot proceed.\n");
        mState = PAIRUP_ERROR;
        return;
    }
    DPRINT("start pairing up wear to phone\n");
    std::vector<std::string> deviceQueue;
    for (size_t i = 0; i < devices.size(); ++i) {
        deviceQueue.push_back(devices[i]);
    }
    mUnprobedDevices.swap(deviceQueue);
    startProbeNextDevice();
}

void PairUpWearPhoneImpl::startConnectWearAndPhone() {
    closeConnection(&mAdbWatch);
    closeConnection(&mConsoleWatch);

    if (mPhoneDevices.empty() || mWearDevices.empty()) {
        startProbeNextDevice();
        return;
    }

    std::string phone = mPhoneDevices[0];
    mPhoneDevices.erase(mPhoneDevices.begin());
    const char emu[] = "emulator-";
    const int sz = sizeof(emu) - 1;
    if (strncmp(emu, phone.c_str(), sz) != 0) {
        // real phone
        if (startWriteCommandToAdb(FORWARDIP, phone.c_str())) {
            mState = TO_FORWARD_IP_SEND_CMD;
        }
    } else {
        // emulator
        mConsolePort = atoi(phone.c_str() + sz);
        if (startReadConsole()) {
            mState = TO_READ_GREETINGS_FROM_CONSOLE;
        }
    }
}

//-----------------------            helper methods

void PairUpWearPhoneImpl::openAdbConnection() {
    mReply.clear();
    if (!openConnection(mAdbHostPort,
                        &mAdbWatch,
                        _on_adb_server_socket_fd)) {
        mState = PAIRUP_ERROR;
    }
}

void PairUpWearPhoneImpl::openConsoleConnection() {
    mReply.clear();
    if (!openConnection(mConsolePort,
                        &mConsoleWatch,
                        _on_emulator_console_socket_fd)) {
        mState = PAIRUP_ERROR;
    }
}

void PairUpWearPhoneImpl::closeConnection(FdWatch* watch) {
    if (watch->get()) {
        (*watch)->dontWantRead();
        (*watch)->dontWantWrite();
        socketClose((*watch)->fd());
        watch->reset(NULL);
    }
}

bool PairUpWearPhoneImpl::openConnection(int port,
                                         FdWatch* watch,
                                         Looper::FdWatch::Callback callback) {
    int so = socketTcp4LoopbackClient(port);
    if (so < 0) {
        DPRINT("failed to open up connection to port %d\n", port);
        watch->reset(NULL);
        return false;
    }
    socketSetNonBlocking(so);
    watch->reset(mLooper->createFdWatch(so, callback, this));
    DPRINT("successfully opened up connection to port %d\n", port);
    return true;
}

// ------------------------------------------------------------------
//
//       PairUpWearPhone  implementation
//
// ------------------------------------------------------------------

PairUpWearPhone::PairUpWearPhone(Looper* looper,
                                 const std::vector<std::string>& devices,
                                 int adbHostPort)
    : mPairUpWearPhoneImpl(
              new PairUpWearPhoneImpl(looper, devices, adbHostPort)) {
    //VERBOSE_ENABLE(adb);
}

PairUpWearPhone::~PairUpWearPhone() {
    delete mPairUpWearPhoneImpl;
}

bool PairUpWearPhone::isDone() const {
    return mPairUpWearPhoneImpl->isDone();
}

} // namespace wear
} // namespace android
