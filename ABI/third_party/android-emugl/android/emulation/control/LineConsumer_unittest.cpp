// Copyright 2015 The Android Open Source Project
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

#include "android/emulation/control/LineConsumer.h"

#include <gtest/gtest.h>
#include <string.h>

using android::emulation::LineConsumer;

TEST(CannedCallbacks, LineConsumerNullTest) {
    LineConsumer lineConsumer;
    EXPECT_TRUE(lineConsumer.opaque() != NULL);
    EXPECT_TRUE(lineConsumer.lines().empty());
}

TEST(CannedCallbacks, LineConsumerMultiLine) {
    const char* kLine0 = {"avasdf3@4%@#%$T\r\n"};
    const char* kLine1 = {"Some line"};
    const char* kLine2 = {"Some other line"};
    const char* kLine3 = {""};
    LineConsumer lineConsumer;
    LineConsumer::Callback(lineConsumer.opaque(), kLine0, strlen(kLine0));
    LineConsumer::Callback(lineConsumer.opaque(), kLine1, strlen(kLine1));
    LineConsumer::Callback(lineConsumer.opaque(), kLine2, strlen(kLine2));
    LineConsumer::Callback(lineConsumer.opaque(), kLine3, strlen(kLine3));

    EXPECT_EQ(4U, lineConsumer.lines().size());
    EXPECT_STREQ(kLine0, lineConsumer.lines()[0].c_str());
    EXPECT_STREQ(kLine1, lineConsumer.lines()[1].c_str());
    EXPECT_STREQ(kLine2, lineConsumer.lines()[2].c_str());
    EXPECT_STREQ(kLine3, lineConsumer.lines()[3].c_str());
}

TEST(CannedCallbacks, LineConsumerBadBuffers) {
    // This buffer is longer than the requested length.
    const char* kLongLine = {"abcdefghijklmn"};
    // This buffer is not NULL terminated.
    const char kEndOfWorld[5] = {'a', 'b', 'c', 'd', 'e'};
    LineConsumer lineConsumer;
    LineConsumer::Callback(lineConsumer.opaque(), kLongLine, 10);
    LineConsumer::Callback(lineConsumer.opaque(), kEndOfWorld, 5);

    EXPECT_EQ(2U, lineConsumer.lines().size());
    EXPECT_STREQ("abcdefghij", lineConsumer.lines()[0].c_str());
    EXPECT_STREQ("abcde", lineConsumer.lines()[1].c_str());
}

// The API with which we use LineConsumer is often C API.
extern "C" {

static void callCallback(void* opaque,
                         LineConsumerCallback callback,
                         const char* str) {
    callback(opaque, str, strlen(str));
}
}

TEST(CannedCallbacks, LineConsumerUseCase) {
    const char* kLine = {"This line"};
    LineConsumer lineConsumer;
    callCallback(lineConsumer.opaque(), &LineConsumer::Callback, kLine);
    EXPECT_EQ(1U, lineConsumer.lines().size());
    EXPECT_STREQ(kLine, lineConsumer.lines()[0].c_str());
}
