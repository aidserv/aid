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

#include "android/base/async/ScopedSocketWatch.h"

#include "android/base/sockets/SocketUtils.h"
#include "android/base/async/ThreadLooper.h"

#include <gtest/gtest.h>

namespace android {
namespace base {

namespace {

struct Data {
    Data() {
        socketCreatePair(&sock[0], &sock[1]);
    }

    ~Data() {
        // Only destroy sock[1] here, since sock0() is supposed to be closed
        // by the test.
        socketClose(sock[1]);
    }

    int sock0() const { return sock[0]; }

    bool tryWrite() {
        char c = '\0';
        return socketSend(sock[0], &c, 1) == 1;
    }

    bool tryRead() {
        char c = '\0';
        return socketRecv(sock[1], &c, 1) == 1;
    }

private:
    int sock[2];
};

}  // namespace

TEST(ScopedSocketWatch, ConstructionAndDestruction) {
    Data data;
    int sock0 = data.sock0();

    {
        ScopedSocketWatch s(ThreadLooper::get()->createFdWatch(
                sock0, nullptr, nullptr));
        EXPECT_TRUE(s.get());
        EXPECT_TRUE(data.tryWrite());
        EXPECT_TRUE(data.tryRead());
        // Exiting the scope will close the socket.
    }

    // The socket is now invalid, writing should fail with EINVAL.
    EXPECT_FALSE(data.tryWrite());
    EXPECT_FALSE(data.tryRead());
}

}  // namespace base
}  // namespace android
