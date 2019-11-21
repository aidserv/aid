// Copyright 2016 The Android Open Source Project
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/event-capturer.h"

EventCapturer::SubscriberToken EventCapturer::subscribeToEvents(
        QObject* root,
        const EventCapturer::ObjectPredicate& child_predicate,
        const EventCapturer::EventTypeSet& event_types,
        const EventCapturer::Callback& callback) {
    // This will contain the set of objects whose events the caller
    // should be subscribed to.
    std::unordered_set<QObject*> objects;

    // Traverse the object hierarchy starting at root. For each object,
    // check the value of |child_predicate|. If it is true, include it
    // into the set of monitored objects.
    auto watch_if_needed = [&child_predicate, &objects, this](QObject* obj) {
        if (child_predicate(obj)) {
            objects.insert(obj);
            obj->installEventFilter(this);
        }
    };
    watch_if_needed(root);
    for (QObject* obj : root->findChildren<QObject*>()) {
        watch_if_needed(obj);
        if (child_predicate(obj)) {
            objects.insert(obj);
            obj->installEventFilter(this);
        }
    }

    return mSubscribers.emplace(event_types, std::move(objects), callback);
}

bool EventCapturer::eventFilter(QObject* target, QEvent* event) {
    // Indicates whether |target| still has any subscribers
    // interested in its events.
    bool has_subscribers = false;

    for (auto& subscriber : mSubscribers) {
        if (subscriber.objects.count(target)) {
            has_subscribers = true;
            if(subscriber.event_types.count(event->type())) {
                subscriber.callback(target, event);
            }
        }
    }

    if (!has_subscribers) {
        // If there are no subscribers interested in events from
        // this object, it is safe to remove the event filter for
        // it.
        target->removeEventFilter(this);
    }

    return QObject::eventFilter(target, event);
}

