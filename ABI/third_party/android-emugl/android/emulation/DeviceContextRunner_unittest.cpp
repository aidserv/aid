// Copyright 2016 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "android/base/async/Looper.h"
#include "android/base/memory/LazyInstance.h"
#include "android/emulation/DeviceContextRunner.h"
#include "android/emulation/testing/TestVmLock.h"
#include "android/emulation/VmLock.h"

#include <gtest/gtest.h>

#include <vector>

using android::base::Looper;
using android::base::LazyInstance;

namespace android {

namespace {

// The DeviceContextRunner unit tests use:
// - a device that just holds the requests sent to it,
//   in order.
struct DeviceContextRunnerTestDevice {
    std::vector<int> requests;
};
//   We keep just one instance of this around:
static LazyInstance<DeviceContextRunnerTestDevice> sTestDevice =
    LAZY_INSTANCE_INIT;

static DeviceContextRunnerTestDevice* getTestDevice() {
    return sTestDevice.ptr();
}
//   And we can reset the requests using this function:
static void resetTestDevice() {
    getTestDevice()->requests.clear();
}
// - An operation type that holds some request code
//   that is sent to the device.
struct DeviceContextRunnerTestOp {
    int request_code;
};
// - A DeviceContextRunner derived class for that operation type,
//   with performDeviceOperation appending the request code
//   to the device's |requests| field:
class TestDeviceContextRunner :
    public DeviceContextRunner<DeviceContextRunnerTestOp> {
public:
    void signal(const DeviceContextRunnerTestOp& op) {
        queueDeviceOperation(op);
    }
private:
    void performDeviceOperation(const DeviceContextRunnerTestOp& op) {
        getTestDevice()->requests.push_back(op.request_code);
    }
};

TEST(DeviceContextRunner, init) {
    resetTestDevice();

    std::unique_ptr<Looper> testLooper(Looper::create());
    TestVmLock testLock;
    TestDeviceContextRunner testRunner;
    testRunner.init(&testLock, testLooper.get());

    EXPECT_EQ(0U, getTestDevice()->requests.size());

    resetTestDevice();
};

TEST(DeviceContextRunner, oneRequest) {
    resetTestDevice();

    std::unique_ptr<Looper> testLooper(Looper::create());

    TestVmLock testLock;

    testLock.lock();

    TestDeviceContextRunner testRunner;
    testRunner.init(&testLock, testLooper.get());

    // We have the lock, so this should immediately
    // go to the device.
    testRunner.signal({ .request_code = 1 });
    EXPECT_EQ(1U, getTestDevice()->requests.size());

    testLock.unlock();

    resetTestDevice();
}

TEST(DeviceContextRunner, oneRequestNeedWait) {
    resetTestDevice();

    std::unique_ptr<Looper> testLooper(Looper::create());

    TestVmLock testLock;

    TestDeviceContextRunner testRunner;
    testRunner.init(&testLock, testLooper.get());

    // We don't have the lock, but the device should
    // get the request after we do have the lock.
    testRunner.signal({ .request_code = 1 });

    testLock.lock();
    testLooper->run();
    testLock.unlock();

    EXPECT_EQ(1U, getTestDevice()->requests.size());

    resetTestDevice();
}

static const size_t kNumRequests = 100;

TEST(DeviceContextRunner, multiRequests) {
    resetTestDevice();

    std::unique_ptr<Looper> testLooper(Looper::create());

    TestVmLock testLock;

    testLock.lock();

    TestDeviceContextRunner testRunner;
    testRunner.init(&testLock, testLooper.get());

    for (size_t i = 0; i < kNumRequests; i++) {
        testRunner.signal({ .request_code = (int)i });
    }

    testLock.unlock();

    EXPECT_EQ(kNumRequests, getTestDevice()->requests.size());

    const std::vector<int>& results =
        getTestDevice()->requests;
    for (size_t i = 0; i < kNumRequests; i++) {
        EXPECT_EQ((int)i, results[i]);
    }

    resetTestDevice();
}

TEST(DeviceContextRunner, multiRequestsNeedWait) {
    resetTestDevice();

    std::unique_ptr<Looper> testLooper(Looper::create());

    TestVmLock testLock;

    TestDeviceContextRunner testRunner;
    testRunner.init(&testLock, testLooper.get());

    for (size_t i = 0; i < kNumRequests; i++) {
        testRunner.signal({ .request_code = (int)i });
    }

    testLock.lock();
    testLooper->run();
    testLock.unlock();

    EXPECT_EQ(kNumRequests, getTestDevice()->requests.size());

    const std::vector<int>& results =
        getTestDevice()->requests;
    for (size_t i = 0; i < kNumRequests; i++) {
        EXPECT_EQ((int)i, results[i]);
    }

    resetTestDevice();
}

} // namespace

} // namespace android
