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

#include "android/skin/qt/event-capturer.h"

#include <QObject>
#include <QPointer>

#include <unordered_map>

// This is a suggested base class for all clients of EventCapturer.
// They're not strictly required to use it, but it makes things
// easier by completely managing subscription/unsubscription.
// Only 3 things are required from clients:
//  1. The logic for processing intercepted events (see
//     |processEvent|.
//  2. A predicate that decides whether this client is
//     interested in monitoring a given object (see |objectPredicate|).
//  3. The set of types of events that the subscriber is interested in
//     (see |eventTypes|).
class EventSubscriber : public QObject {
Q_OBJECT
public:
    // Creates a new subscriber based on the given EventCapturer.
    explicit EventSubscriber(EventCapturer* ecap);
    virtual ~EventSubscriber(){}

public slots:
    // Starts monitoring events from the hierarchy of a given object.
    // When the given object is destroyed, unsubscription will happen
    // automatically.
    void startRecording(QObject* target);

    // Stops monitoring events from the given object's hierarchy.
    // This method has no effect if the subscriber was not previously
    // subscribed to the events from that object.
    void stopRecording(QObject* target);

    // The effect of this slot is equivalent to calling the previous method
    // with its parameter set to the object that emitted the triggering signal.
    // This method has no effect when called directly.
    void stopRecordingForEmitter();

private:
    // Child classes should implement their event processing logic in this
    // method.
    virtual void processEvent(const QObject* target, const QEvent* event) = 0;

    // Child classes should implement this method to return true if they're
    // interested in intercepting events from the given object.
    virtual bool objectPredicate(const QObject* object) const = 0;

    // Child classes should implement this method to return the set of types
    // of events they're interested in.
    virtual const EventCapturer::EventTypeSet& eventTypes() const = 0;

private:
    QPointer<EventCapturer> mEventCapturer;
    std::unordered_map<QObject*, EventCapturer::SubscriberToken> mTokens;
};

