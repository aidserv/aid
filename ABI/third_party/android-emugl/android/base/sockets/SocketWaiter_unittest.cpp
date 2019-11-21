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

#include "android/base/sockets/SocketUtils.h"
#include "android/base/sockets/SocketWaiter.h"
#include "android/base/memory/ScopedPtr.h"

#include <gtest/gtest.h>

namespace android {
namespace base {

// Check that initialization / destruction works without crashing.
TEST(SocketWaiter, init) {
    ScopedPtr<SocketWaiter> waiter(SocketWaiter::create());

    unsigned events = ~0U;
    EXPECT_EQ(-1, waiter->nextPendingFd(&events));
    EXPECT_EQ(0U, events);
}

TEST(SocketWaiter, reset) {
    ScopedPtr<SocketWaiter> waiter(SocketWaiter::create());

    EXPECT_FALSE(waiter->hasFds());

    int s1, s2;

    ASSERT_EQ(0, socketCreatePair(&s1, &s2));

    waiter->update(s1, SocketWaiter::kEventRead);
    EXPECT_TRUE(waiter->hasFds());
    EXPECT_EQ(SocketWaiter::kEventRead, waiter->wantedEventsFor(s1));

    waiter->reset();
    EXPECT_FALSE(waiter->hasFds());
    EXPECT_EQ(0U, waiter->wantedEventsFor(s1));
}

TEST(SocketWaiter, update) {
    ScopedPtr<SocketWaiter> waiter(SocketWaiter::create());

    int s1, s2;

    ASSERT_EQ(0, socketCreatePair(&s1, &s2));

    waiter->update(s1, SocketWaiter::kEventRead);
    waiter->update(s2, SocketWaiter::kEventWrite);
    EXPECT_TRUE(waiter->hasFds());
    EXPECT_EQ(SocketWaiter::kEventRead, waiter->wantedEventsFor(s1));
    EXPECT_EQ(SocketWaiter::kEventWrite, waiter->wantedEventsFor(s2));

    waiter->update(s1, SocketWaiter::kEventWrite);
    waiter->update(s2, SocketWaiter::kEventRead);
    EXPECT_TRUE(waiter->hasFds());
    EXPECT_EQ(SocketWaiter::kEventWrite, waiter->wantedEventsFor(s1));
    EXPECT_EQ(SocketWaiter::kEventRead, waiter->wantedEventsFor(s2));

    waiter->update(s1, SocketWaiter::kEventRead | SocketWaiter::kEventWrite);
    waiter->update(s2, SocketWaiter::kEventRead | SocketWaiter::kEventWrite);
    EXPECT_TRUE(waiter->hasFds());
    EXPECT_EQ((unsigned)(SocketWaiter::kEventRead | SocketWaiter::kEventWrite),
              waiter->wantedEventsFor(s1));
    EXPECT_EQ((unsigned)(SocketWaiter::kEventRead | SocketWaiter::kEventWrite),
              waiter->wantedEventsFor(s2));

    waiter->reset();

    EXPECT_FALSE(waiter->hasFds());
    EXPECT_EQ(0U, waiter->wantedEventsFor(s1));
    EXPECT_EQ(0U, waiter->wantedEventsFor(s2));
}

TEST(SocketWaiter, waitOnReadEvent) {
    ScopedPtr<SocketWaiter> waiter(SocketWaiter::create());

    int s1, s2;

    ASSERT_EQ(0, socketCreatePair(&s1, &s2));

    waiter->update(s1, SocketWaiter::kEventRead);

    EXPECT_EQ(1, socketSend(s2, "!", 1));
    int ret = waiter->wait(0);
    EXPECT_EQ(1, ret);
    unsigned events = 0;
    EXPECT_EQ(s1, waiter->nextPendingFd(&events));
    EXPECT_EQ(SocketWaiter::kEventRead, events);

    EXPECT_EQ(-1, waiter->nextPendingFd(&events));

    socketClose(s2);
    socketClose(s1);
}

TEST(SocketWaiter, waitOnWriteEvent) {
    ScopedPtr<SocketWaiter> waiter(SocketWaiter::create());

    int s1, s2;

    ASSERT_EQ(0, socketCreatePair(&s1, &s2));

    waiter->update(s2, SocketWaiter::kEventWrite);
    int ret = waiter->wait(0);
    EXPECT_EQ(1, ret);
    unsigned events = 0;
    EXPECT_EQ(s2, waiter->nextPendingFd(&events));
    EXPECT_EQ(SocketWaiter::kEventWrite, events);

    EXPECT_EQ(-1, waiter->nextPendingFd(&events));

    socketClose(s2);
    socketClose(s1);
}


}  // namespace base
}  // namespace android
