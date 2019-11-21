// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public License
// version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include "android/skin/qt/event-subscriber.h"

#include <QObject>

namespace android {
namespace qt {

class UserActionsCounter : public QObject {
    Q_OBJECT

public:
    UserActionsCounter(EventCapturer* eventCapturer);

public slots:
    // Different window types are treated slightly differently.
    // For UI widget containing windows, we are only interested in the button
    // clicks and other widget related actions.
    // On windows that represent the virtual device itself, all user clicks are
    // significant.
    void startCountingForMainWindow(QObject* target) {
        startCountingAll(target);
    }
    void startCountingForOverlayWindow(QObject* target) {
        startCountingAll(target);
    }
    void startCountingForToolWindow(QObject* target) {
        startCountingMarked(target);
    }
    void startCountingForExtendedWindow(QObject* target) {
        startCountingMarked(target);
    }

private slots:
    // Stops monitoring events from the given target's hierarchy.  This method
    // has no effect if the subscriber was not previously subscribed to the
    // events from that target.
    // This must be connected to a signal from the target. Calling it directly
    // (as a function) has no effect.
    void stopCountingForEmitter();

public:
    // Reset the user action count. Does not stop listening for events on the
    // registered objects.
    void reset() { mCount = 0; }
    uint64_t count() const { return mCount; }

private:
    static const char kReportMetricsProperty[];
    void startCountingAll(QObject* target);
    void startCountingMarked(QObject* target);
    static bool isMarked(const QObject* target);
    void processEvent(const QObject* target, const QEvent* event);

    const EventCapturer::EventTypeSet mEventTypes;
    EventCapturer* mEventCapturer;
    std::unordered_map<QObject*, EventCapturer::SubscriberToken> mTokens;
    uint64_t mCount = 0;
};

}  // namespace qt
}  // namespace android
