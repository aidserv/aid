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

#include "android/base/async/Looper.h"

#include "android/base/Log.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/base/sockets/ScopedSocket.h"
#include <gtest/gtest.h>

#include <memory>

#include <errno.h>

namespace android {
namespace base {

static bool createScopedSocketPair(ScopedSocket* s0, ScopedSocket* s1) {
    int fd0 = -1, fd1 = -1;
    if (socketCreatePair(&fd0, &fd1) < 0) {
        PLOG(ERROR) << "Could not create socket pair";
        return false;
    }
    s0->reset(fd0);
    s1->reset(fd1);
    return true;
}

using Duration = Looper::Duration;

TEST(GenericLooper, CreationAndDestruction) {
    std::unique_ptr<Looper> looper(Looper::create());
    ASSERT_TRUE(looper.get());
}

TEST(GenericLooper, RunWithoutHandlers) {
    std::unique_ptr<Looper> looper(Looper::create());

    // A simple run() should return immediately. Accept 5 ms max, which is
    // huge but might account for overloaded machines when the tests are run.
    const Duration kMaxDelayMs = 5;
    Duration now0 = looper->nowMs();
    looper->run();
    Duration now1 = looper->nowMs();
    EXPECT_LE(now1 - now0, kMaxDelayMs);

    // Similarly, runWithDeadlineMs(kInfiniteDuration) should return
    // EWOULDBLOCK.
    now0 = looper->nowMs();
    EXPECT_EQ(EWOULDBLOCK, looper->runWithDeadlineMs(
            Looper::kDurationInfinite));
    now1 = looper->nowMs();
    EXPECT_LE(now1 - now0, kMaxDelayMs);

    // Same with an arbitrary fixed deadline.
    now0 = looper->nowMs();
    EXPECT_EQ(EWOULDBLOCK, looper->runWithDeadlineMs(now0 + 1000));
    now1 = looper->nowMs();
    EXPECT_LE(now1 - now0, kMaxDelayMs);
}

static void myTimerCallbackNoop(void* opaque, Looper::Timer* timer) {}

static void myTimerCallbackForceQuit(void* opaque, Looper::Timer* timer) {
    auto looper = static_cast<Looper*>(opaque);
    looper->forceQuit();
}

static void myTimerCallbackSelfDelete(void* opaque, Looper::Timer* timer) {
    delete timer;
}

namespace {

struct TimerGroup {
    Looper* looper;
    Looper::Timer* timer1;
    Looper::Timer* timer2;
};

}  // namespace

static void myTimerCallbackStopSecondTimer(void* opaque,
                                           Looper::Timer* timer) {
    auto group = static_cast<TimerGroup*>(opaque);
    ASSERT_EQ(timer, group->timer1);
    group->timer2->stop();
}

static void myTimerCallbackShouldNeverBeCalled(void* opaque,
                                               Looper::Timer* timer) {
    GTEST_FAIL() << "This timer callback should never be called!!";
}

TEST(GenericLooper, RunWithSingleTimer) {
    std::unique_ptr<Looper> looper(Looper::create());

    // Create a timer that runs after 10ms then calls Looper::forceQuit().
    std::unique_ptr<Looper::Timer> timer1(looper->createTimer(
            myTimerCallbackNoop, looper.get()));
    ASSERT_TRUE(timer1.get());
    ASSERT_EQ(looper.get(), timer1->parentLooper());
    EXPECT_FALSE(timer1->isActive());

    const Duration kDelayMs = 10;
    Duration now0 = looper->nowMs();
    timer1->startAbsolute(now0 + kDelayMs);
    EXPECT_TRUE(timer1->isActive());
    int ret = looper->runWithDeadlineMs(Looper::kDurationInfinite);
    EXPECT_EQ(EWOULDBLOCK, ret);
    Duration now1 = looper->nowMs();
    EXPECT_GE(now1 - now0, kDelayMs);
    EXPECT_FALSE(timer1->isActive());
}

TEST(GenericLooper, RunWithSingleTimerTimingOut) {
    std::unique_ptr<Looper> looper(Looper::create());

    // Create a timer that runs after 10ms then calls Looper::forceQuit().
    std::unique_ptr<Looper::Timer> timer1(looper->createTimer(
            myTimerCallbackNoop, looper.get()));
    ASSERT_TRUE(timer1.get());
    ASSERT_EQ(looper.get(), timer1->parentLooper());
    EXPECT_FALSE(timer1->isActive());

    const Duration kDelayMs = 10;
    Duration now0 = looper->nowMs();
    timer1->startAbsolute(now0 + kDelayMs);
    EXPECT_TRUE(timer1->isActive());
    int ret = looper->runWithDeadlineMs(now0 + kDelayMs / 2);
    EXPECT_EQ(ETIMEDOUT, ret);
    Duration now1 = looper->nowMs();
    EXPECT_GE(now1 - now0, kDelayMs / 2);
    EXPECT_LT(now1 - now0, kDelayMs);
    EXPECT_TRUE(timer1->isActive());
}

TEST(GenericLooper, RunWithSingleTimerForceQuitting) {
    std::unique_ptr<Looper> looper(Looper::create());

    // Create a timer that runs after 10ms then calls Looper::forceQuit().
    std::unique_ptr<Looper::Timer> timer1(looper->createTimer(
            myTimerCallbackForceQuit, looper.get()));
    ASSERT_TRUE(timer1.get());
    ASSERT_EQ(looper.get(), timer1->parentLooper());
    EXPECT_FALSE(timer1->isActive());

    const Duration kDelayMs = 10;
    Duration now0 = looper->nowMs();
    timer1->startAbsolute(now0 + kDelayMs);
    EXPECT_TRUE(timer1->isActive());
    looper->run();
    Duration now1 = looper->nowMs();
    EXPECT_GE(now1 - now0, kDelayMs);
    EXPECT_FALSE(timer1->isActive());
}

TEST(GenericLooper, RunWithSingleTimerSelfDeleting) {
    std::unique_ptr<Looper> looper(Looper::create());

    // Create a timer that runs after 10ms then self deletes itself, which
    // will force run() to exit.
    Looper::Timer* timer1 = looper->createTimer(
            myTimerCallbackSelfDelete, looper.get());
    ASSERT_TRUE(timer1);
    ASSERT_EQ(looper.get(), timer1->parentLooper());

    const Duration kDelayMs = 10;
    Duration now0 = looper->nowMs();
    timer1->startAbsolute(now0 + kDelayMs);
    EXPECT_TRUE(timer1->isActive());
    looper->run();
    Duration now1 = looper->nowMs();
    EXPECT_GE(now1 - now0, kDelayMs);
    // TIMER WAS DELETED: EXPECT_FALSE(timer1->isActive());
}

TEST(GenericLooper, RunWithTwoTimersOneStoppingTheOther) {
    // Create two timers, one that runs after 10ms, and the other after 20ms
    // The first timer callback will stop the second one, which will force
    // Looper::run() to exit after 10ms.
    std::unique_ptr<Looper> looper(Looper::create());

    TimerGroup group = { .looper = looper.get(), };

    // Create a timer that runs after 10ms then calls Looper::forceQuit().
    std::unique_ptr<Looper::Timer> timer1(looper->createTimer(
            myTimerCallbackStopSecondTimer, &group));
    ASSERT_TRUE(timer1.get());
    ASSERT_EQ(looper.get(), timer1->parentLooper());
    group.timer1 = timer1.get();

    std::unique_ptr<Looper::Timer> timer2(looper->createTimer(
            myTimerCallbackShouldNeverBeCalled, looper.get()));
    ASSERT_TRUE(timer1.get());
    ASSERT_EQ(looper.get(), timer2->parentLooper());
    group.timer2 = timer2.get();

    const Duration kDelayMs = 10;
    Duration now0 = looper->nowMs();
    timer1->startAbsolute(now0 + kDelayMs);
    EXPECT_TRUE(timer1->isActive());
    timer2->startAbsolute(now0 + 2 * kDelayMs);
    EXPECT_TRUE(timer2->isActive());
    looper->run();
    Duration now1 = looper->nowMs();
    EXPECT_GE(now1 - now0, kDelayMs);
    EXPECT_LT(now1 - now0, 2 * kDelayMs);
    EXPECT_FALSE(timer1->isActive());
    EXPECT_FALSE(timer2->isActive());
}

static void myFdWatchCallbackWriteBooleanFlag(void* opaque, int fd,
                                              unsigned events) {
    EXPECT_EQ(Looper::FdWatch::kEventRead, events);
    auto pflag = static_cast<bool*>(opaque);
    *pflag = true;
}

TEST(GenericLooper, RunWithSingleFdWatchWithTimeout) {
    ScopedSocket s0;
    ScopedSocket s1;
    ASSERT_TRUE(createScopedSocketPair(&s0, &s1));

    std::unique_ptr<Looper> looper(Looper::create());

    // Create a singl FdWatch to set a boolean flag on socket event,
    // but do not activate it (i.e. do not call ::wantRead()), the
    // looper should consider there are no active handle and return
    // immediately from its loop with EWOULDBLOCK.
    bool flag = false;
    std::unique_ptr<Looper::FdWatch> watch1(
        looper->createFdWatch(s1.get(),
                              myFdWatchCallbackWriteBooleanFlag,
                              &flag));
    ASSERT_TRUE(watch1.get());

    EXPECT_EQ(0U, watch1->poll());
    EXPECT_FALSE(flag);

    socketSend(s0.get(), "x", 1);

    const Duration kDelayMs = 10;
    Duration now0 = looper->nowMs();
    int ret = looper->runWithDeadlineMs(now0 + kDelayMs);
    Duration now1 = looper->nowMs();
    EXPECT_EQ(ETIMEDOUT, ret);
    EXPECT_LT(now0 - now1, kDelayMs);
    EXPECT_FALSE(flag);  // Callback was not called.
}

TEST(GenericLooper, RunWithSingleFdWatchSettingBooleanFlag) {
    ScopedSocket s0;
    ScopedSocket s1;
    ASSERT_TRUE(createScopedSocketPair(&s0, &s1));

    std::unique_ptr<Looper> looper(Looper::create());

    // Create a singl FdWatch to set a boolean flag on socket event,
    // and activate it. Verify that the looper returns before a max delay
    // of 100ms due to the event.
    bool flag = false;
    std::unique_ptr<Looper::FdWatch> watch1(
        looper->createFdWatch(s1.get(),
                              myFdWatchCallbackWriteBooleanFlag,
                              &flag));
    ASSERT_TRUE(watch1.get());

    EXPECT_EQ(0U, watch1->poll());
    EXPECT_FALSE(flag);

    socketSend(s0.get(), "x", 1);
    watch1->wantRead();

    const Duration kDelayMs = 100;
    Duration now0 = looper->nowMs();
    int ret = looper->runWithDeadlineMs(now0 + kDelayMs);
    Duration now1 = looper->nowMs();
    EXPECT_EQ(ETIMEDOUT, ret);
    EXPECT_LT(now0 - now1, kDelayMs);
    EXPECT_TRUE(flag);  // Callback was called.
}

static void myTimerCallbackRemoveFdWatchRead(void* opaque,
                                             Looper::Timer* timer) {
    auto watch = static_cast<Looper::FdWatch*>(opaque);
    ASSERT_EQ(Looper::FdWatch::kEventRead, watch->poll());
    watch->dontWantRead();
}

TEST(GenericLooper, RunTimerAndFdWatchWithTimerRemovingPendingWatch) {
    ScopedSocket s0;
    ScopedSocket s1;
    ASSERT_TRUE(createScopedSocketPair(&s0, &s1));

    std::unique_ptr<Looper> looper(Looper::create());

    // Create a singl FdWatch to set a boolean flag on socket event,
    // and activate it. Verify that the looper returns before a max delay
    // of 100ms due to the event.
    bool flag = false;
    std::unique_ptr<Looper::FdWatch> watch1(
            looper->createFdWatch(s1.get(),
                                  myFdWatchCallbackWriteBooleanFlag,
                                  &flag));
    ASSERT_TRUE(watch1.get());

    EXPECT_EQ(0U, watch1->poll());
    EXPECT_FALSE(flag);
    watch1->wantRead();

    // Write a byte to the socket pair, this will make the watch pending
    // for the next Looper::run() call.
    socketSend(s0.get(), "x", 1);

    // Create a new timer that will be fired immediately and will remove the
    // pending watch.
    std::unique_ptr<Looper::Timer> timer1(
            looper->createTimer(myTimerCallbackRemoveFdWatchRead,
                                watch1.get()));


    const Duration kDelayMs = 10;
    Duration now0 = looper->nowMs();
    timer1->startAbsolute(now0);
    int ret = looper->runWithDeadlineMs(now0 + kDelayMs);
    Duration now1 = looper->nowMs();
    EXPECT_EQ(ETIMEDOUT, ret);
    EXPECT_LT(now0 - now1, kDelayMs);
    EXPECT_FALSE(flag);  // Callback was not called.
}

TEST(GenericLooper, CreateTask) {
    std::unique_ptr<Looper> looper(Looper::create());

    bool taskRan = false;
    auto task = looper->createTask([&taskRan]() { taskRan = true; });
    ASSERT_TRUE(task);

    EXPECT_EQ(EWOULDBLOCK,
              looper->runWithDeadlineMs(Looper::kDurationInfinite));
    EXPECT_FALSE(taskRan);

    task->schedule();
    EXPECT_EQ(EWOULDBLOCK,
              looper->runWithDeadlineMs(Looper::kDurationInfinite));
    EXPECT_TRUE(taskRan);

    taskRan = false;
    EXPECT_EQ(EWOULDBLOCK,
              looper->runWithDeadlineMs(Looper::kDurationInfinite));

    task->schedule();
    task->cancel();
    EXPECT_EQ(EWOULDBLOCK,
              looper->runWithDeadlineMs(Looper::kDurationInfinite));
    EXPECT_EQ(EWOULDBLOCK,
              looper->runWithDeadlineMs(Looper::kDurationInfinite));
    EXPECT_FALSE(taskRan);

    task->schedule();
    EXPECT_EQ(EWOULDBLOCK,
              looper->runWithDeadlineMs(Looper::kDurationInfinite));
    EXPECT_TRUE(taskRan);
}

TEST(GenericLooper, ScheduleCallback) {
    std::unique_ptr<Looper> looper(Looper::create());

    bool taskRan = false;
    looper->scheduleCallback([&taskRan]() { taskRan = true; });

    EXPECT_EQ(EWOULDBLOCK,
              looper->runWithDeadlineMs(Looper::kDurationInfinite));
    EXPECT_TRUE(taskRan);

    taskRan = false;
    EXPECT_EQ(EWOULDBLOCK,
              looper->runWithDeadlineMs(Looper::kDurationInfinite));
    EXPECT_FALSE(taskRan);
}

}  // namespace base
}  // namespace android
