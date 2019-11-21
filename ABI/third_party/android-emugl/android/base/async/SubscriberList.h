// Copyright 2016 The Android Open Source Project
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include "android/base/Compiler.h"

#include <functional>
#include <list>
#include <memory>

#include <assert.h>

namespace android {
namespace base {

// Any object that intends to notify certain subscribers on the event loop can
// use the SubscriberList to keep track of all active clients. The clients in
// turn, must keep the returned SubscriptionToken alive for as long as they want
// to be notified of updates.
//
// Once a SubscriptionToken is destroyed, the SubscriptionList removes that
// client from future notifications.
// If the SubscriptionList is destroyed while some tokens are still active,
// destryong them no longer has any effect.
//
// See SubscriberList_unittest.cpp -- TestSubscription and TestClient -- for an
// example of how to use this.
//
// These objects are not thread safe. These operations MUST be done on the same
// thread:
// - |insert| / |emplace| new subscriptions.
// - Destroy a returned SubscriptionToken.
// - Destroy the SubscriberList.
namespace internal {
// Helper class that stores a callable object and invokes it upon destruction
// (unless invalidate() has been called).
class SubscriptionTokenImpl {
public:
    using Callback = std::function<void()>;

    explicit SubscriptionTokenImpl(const Callback& cb) : mCb(cb) {}

    ~SubscriptionTokenImpl() {
        if (mCb)
            mCb();
    }

    // If this method is called, the stored callable object
    // will not be invoked upon destruction.
    void invalidate() { mCb = Callback(); }

private:
    Callback mCb;
    DISALLOW_COPY_ASSIGN_AND_MOVE(SubscriptionTokenImpl);
};
}  // namespace internal

using SubscriptionToken = std::unique_ptr<internal::SubscriptionTokenImpl>;

template <class SubscriberInfo>
class SubscriberList {
private:
    // DO NOT CHANGE TO std::vector (or any other type of container
    // that invalidates iterators on removal)!
    // A list of objects that our clients care about.
    using SubscriberInfoList = std::list<SubscriberInfo>;
    using SubscriberInfoIterator = typename SubscriberInfoList::iterator;
    // A list for internal bookkeeping.
    typedef struct {
        internal::SubscriptionTokenImpl* token;
        SubscriberInfoIterator infoIterator;
    } MetaData;
    using MetaDataList = std::list<MetaData>;
    using MetaDataListIterator = typename MetaDataList::iterator;

public:
    using const_iterator = typename SubscriberInfoList::const_iterator;

    SubscriberList() = default;

    template <class... Args>
    SubscriptionToken emplace(Args&&... args) {
        mSubscriberInfos.emplace_back(std::forward<Args>(args)...);
        return insertInternal(std::prev(mSubscriberInfos.end()));
    }

    SubscriptionToken insert(SubscriberInfo&& info) {
        mSubscriberInfos.push_back(std::move(info));
        return insertInternal(std::prev(mSubscriberInfos.end()));
    }

    SubscriptionToken insert(const SubscriberInfo& info) {
        mSubscriberInfos.push_back(info);
        return insertInternal(std::prev(mSubscriberInfos.end()));
    }

    // Your gateway to the subscribers.
    // These iterators can be invalidated as subscribers are destroyed. Do not
    // capture the iterators for asynchronous use.
    const_iterator begin() const { return mSubscriberInfos.begin(); }
    const_iterator end() const { return mSubscriberInfos.end(); }
    size_t size() const { return mSubscriberInfos.size(); }

    ~SubscriberList() {
        // Invalidate all the tokens dispensed by this instance.  Failing to do
        // that would cause the tokens to try and call into this instance upon
        // destruction, resulting in undefined behavior.
        for (const auto& subscriber : mMetaData) {
            // No way the token can be NULL.
            assert(subscriber.token);
            subscriber.token->invalidate();
        }
    }

private:
    void unsubscribe(MetaDataListIterator iterator) {
        mSubscriberInfos.erase(iterator->infoIterator);
        mMetaData.erase(iterator);
    }

    SubscriptionToken insertInternal(SubscriberInfoIterator infoIterator) {
        // Add an entry with information needed for subscriber book-keeping.
        mMetaData.emplace_back();
        auto datum = std::prev(mMetaData.end());
        datum->infoIterator = infoIterator;

        // Create a new token that calls |unsubscribe| upon destruction.
        SubscriptionToken token(new internal::SubscriptionTokenImpl(
                [this, datum] { this->unsubscribe(datum); }));
        // Ensure that this instance can invalidate the token.
        datum->token = token.get();

        // Give the token to the subscriber.
        // The token will remain alive for as long as the subscriber that is
        // holding on to it is alive.
        // If the token is destroyed before the SubscriptionList is destroyed,
        // it will notify us, and the corresponding subscriber entry will be
        // removed.
        // When the SubscriptionList is destroyed, it will go through all the
        // remaining subscriber entries and invalidate their tokens, so that
        // they don't try notifying the "dead" SubscriptionList.
        return token;
    }

    SubscriberInfoList mSubscriberInfos;
    MetaDataList mMetaData;

    DISALLOW_COPY_ASSIGN_AND_MOVE(SubscriberList);
};

}  // namespace base
}  // namespace android
