// Copyright 2016 The Android Open Source Project
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/event-subscriber.h"

EventSubscriber::EventSubscriber(EventCapturer* ecap) : mEventCapturer(ecap) {}

void EventSubscriber::startRecording(QObject* target) {
    if (mEventCapturer) {
        mTokens[target] = mEventCapturer->subscribeToEvents(
                target,
                [this](const QObject* o){ return this->objectPredicate(o); },
                eventTypes(),
                [this](const QObject* target, const QEvent* event) {
                    this->processEvent(target, event);
                });
        connect(target, SIGNAL(destroyed()), this, SLOT(stopRecordingForEmitter()));
    }
}

void EventSubscriber::stopRecording(QObject* target) {
    mTokens.erase(target);
}

void EventSubscriber::stopRecordingForEmitter() {
    if (auto target = sender()) {
        stopRecording(target);
    }
}

