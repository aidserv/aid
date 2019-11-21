// Copyright 2016 The Android Open Source Project
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/async/SubscriberList.h"

#include <gtest/gtest.h>

#include <memory>

using android::base::SubscriberList;
using android::base::SubscriptionToken;
using std::unique_ptr;

namespace {
// Everything is public. Life is easy.
class TestClient {
public:
    void notify() { ++mCalls; }
    int mCalls = 0;
};

class TestFeed {
public:
    // This struct is purely instructional.
    struct SubscriberInfo {
        TestClient* client;
        SubscriberInfo(TestClient* c) : client(c) {}
    };

    using SubscriberList = android::base::SubscriberList<SubscriberInfo>;

    SubscriptionToken subscribe(TestClient* client) {
        return mSubscribers.emplace(client);
    }

    SubscriptionToken subscribeViaInsert(TestClient* client) {
        SubscriberInfo info(client);
        return mSubscribers.insert(info);
    }

    void notifyAll() {
        for (auto& subscriber : mSubscribers) {
            subscriber.client->notify();
        }
    }

    SubscriberList mSubscribers;
};
}  // namespace

TEST(SubscriberList, insert) {
    TestClient client;
    TestFeed feed;

    auto token = feed.subscribeViaInsert(&client);
    feed.notifyAll();
    EXPECT_EQ(1, client.mCalls);
    feed.notifyAll();
    EXPECT_EQ(2, client.mCalls);

    token.reset();
    feed.notifyAll();
    EXPECT_EQ(2, client.mCalls);
}

TEST(SubscriberList, emplace) {
    TestClient client;
    TestFeed feed;

    auto token = feed.subscribe(&client);
    feed.notifyAll();
    EXPECT_EQ(1, client.mCalls);
    feed.notifyAll();
    EXPECT_EQ(2, client.mCalls);

    token.reset();
    feed.notifyAll();
    EXPECT_EQ(2, client.mCalls);
}

TEST(SubscriberList, deadList) {
    TestClient client1, client2;
    unique_ptr<TestFeed> feed(new TestFeed());

    auto token1 = feed->subscribe(&client1);
    auto token2 = feed->subscribe(&client2);

    feed->notifyAll();
    EXPECT_EQ(1, client1.mCalls);
    EXPECT_EQ(1, client2.mCalls);
    token1.reset();
    feed->notifyAll();
    EXPECT_EQ(1, client1.mCalls);
    EXPECT_EQ(2, client2.mCalls);

    feed.reset();
    // This _may_ crash if our implementation doesn't invalidate tokens
    // properly. Unfortunately there isn't a way to make this crash every time.
    token2.reset();
}
