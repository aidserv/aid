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

#include "android/base/async/SubscriberList.h"
#include "android/base/Compiler.h"
#include "android/skin/qt/qt-std-hash.h"

#include <QEvent>
#include <QObject>

#include <list>
#include <memory>
#include <unordered_set>

// Use this class to "spy" on events received by other objects.
// This class is not thread-safe. All subscribers should be on
// the UI thread.
class EventCapturer : public QObject {
Q_OBJECT
public:
    using Callback = std::function<void(const QObject*, const QEvent*)>;
    using ObjectPredicate = std::function<bool(const QObject*)>;
    using EventTypeSet = std::unordered_set<QEvent::Type>;
    using SubscriberToken = android::base::SubscriptionToken;

private:
    struct SubscriberInfo {
        // What types of events the subscriber wants.
        EventTypeSet event_types;

        // What objects the subscriber wants to monitor.
        std::unordered_set<QObject*> objects;

        // Callback provided by the subscriber.
        Callback callback;

        SubscriberInfo(EventTypeSet e,
                       std::unordered_set<QObject*> o,
                       Callback c)
            : event_types(e), objects(o), callback(c) {}
    };

public:
    EventCapturer() = default;
    virtual ~EventCapturer() = default;

    // Ensures that |callback| is invoked for every event that meets
    // the following requirements:
    //   1. The event is triggered for an object that is part
    //      of |root|'s hierarchy.
    //   2. |child_filter| was true for that object at the moment
    //      |subscribeToEvents| was called.
    //   3. The event's type is one of those listed in |event_types|.
    // This method returns a non-copyable, movable, opaque token.
    // When that token is destroyed (i.e. by going out of scope),
    // events will no longer be delivered to the subscriber.
    SubscriberToken subscribeToEvents(
            QObject* root,
            const ObjectPredicate& child_filter,
            const EventTypeSet& event_types,
            const Callback& callback);

private:
    // Intercepts the events from monitored objects.
    bool eventFilter(QObject* target, QEvent* event) override;

    // Information about all the subscribers.
    android::base::SubscriberList<SubscriberInfo> mSubscribers;

    DISALLOW_COPY_AND_ASSIGN(EventCapturer);
};

