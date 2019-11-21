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

#include "android/emulation/testing/TestAndroidPipeDevice.h"

#include "android/emulation/AndroidPipe.h"
#include "android/base/Log.h"

#include <errno.h>
#include <stdint.h>
#include <string.h>

namespace android {

namespace {

class TestGuest : public TestAndroidPipeDevice::Guest {
public:
    TestGuest() : mClosed(true), mWakes(0u), mPipe(nullptr) {
        mPipe = android_pipe_guest_open(this);
        if (!mPipe) {
            LOG(ERROR) << "Could not create new "
                          "TestAndroidPipeDevice::Guest instance!";
        }
    }

    virtual ~TestGuest() { close(); }

    virtual int connect(const char* name) override {
        std::string handshake("pipe:");
        handshake += name;
        int len = static_cast<int>(handshake.size()) + 1;
        mClosed = false;
        int ret = write(handshake.c_str(), len);
        if (ret != len) {
            LOG(ERROR) << "Could not connect to service " << name
                       << " ret=" << ret << " expected len=" << len;
            mClosed = true;
            return -EINVAL;
        }
        return 0;
    }

    virtual ssize_t read(void* buffer, size_t len) override {
        if (mClosed) {
            return 0;
        }
        AndroidPipeBuffer buf = { static_cast<uint8_t*>(buffer), len };
        return android_pipe_guest_recv(mPipe, &buf, 1);
    }

    virtual ssize_t write(const void* buffer, size_t len) override {
        if (mClosed) {
            return 0;
        }
        AndroidPipeBuffer buf = {
                (uint8_t*)buffer, len };
        return android_pipe_guest_send(mPipe, &buf, 1);
    }

    virtual void close() override {
        if (!mClosed) {
            mClosed = true;
            android_pipe_guest_close(mPipe, PIPE_CLOSE_GRACEFUL);
        }
    }

    virtual unsigned poll() const override {
        if (mClosed) {
            return PIPE_POLL_HUP;
        }
        return android_pipe_guest_poll(mPipe);
    }

    virtual void* getPipe() const override { return mPipe; }

    void resetPipe(void* internal_pipe) {
        mPipe = internal_pipe;
    }

    void closeFromHost() {
        mClosed = true;
    }

    void signalWake(int wakes) {
        // NOTE: Update the flags, but for now don't do anything
        //       about them.
        mWakes |= wakes;
    }

private:
    bool mClosed;
    unsigned mWakes;
    void* mPipe;
};

}  // namespace

TestAndroidPipeDevice::TestAndroidPipeDevice()
        : mOldHwFuncs(android_pipe_set_hw_funcs(&sHwFuncs)) {
    AndroidPipe::Service::resetAll();
    AndroidPipe::initThreading(&mVmLock);
    mVmLock.lock();
}

TestAndroidPipeDevice::~TestAndroidPipeDevice() {
    android_pipe_set_hw_funcs(mOldHwFuncs);
    AndroidPipe::Service::resetAll();
    mVmLock.unlock();
}

// static
const AndroidPipeHwFuncs TestAndroidPipeDevice::sHwFuncs = {
    &TestAndroidPipeDevice::resetPipe,
    &TestAndroidPipeDevice::closeFromHost,
    &TestAndroidPipeDevice::signalWake,
};

// static
void TestAndroidPipeDevice::resetPipe(void* hwpipe, void* internal_pipe) {
    static_cast<TestGuest*>(hwpipe)->resetPipe(internal_pipe);
}

// static
void TestAndroidPipeDevice::closeFromHost(void* hwpipe) {
    static_cast<TestGuest*>(hwpipe)->closeFromHost();
}

// static
void TestAndroidPipeDevice::signalWake(void* hwpipe, unsigned wakes) {
    static_cast<TestGuest*>(hwpipe)->signalWake(wakes);
}

// static
TestAndroidPipeDevice::Guest* TestAndroidPipeDevice::Guest::create() {
    return new TestGuest();
}

}  // namespace android
