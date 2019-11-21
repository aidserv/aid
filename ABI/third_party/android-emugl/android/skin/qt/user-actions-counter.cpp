// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public License
// version 2, as published by the Free Software Foundation, and may be copied,
// distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/user-actions-counter.h"

#include <QVariant>

#include <assert.h>

namespace android {
namespace qt {

// static
const char UserActionsCounter::kReportMetricsProperty[] = "report_metrics";

UserActionsCounter::UserActionsCounter(EventCapturer* eventCapturer)
        : mEventTypes({QEvent::MouseButtonRelease, QEvent::KeyRelease,
                       QEvent::FileOpen}),
          mEventCapturer(eventCapturer) {
    assert(mEventCapturer);
}

void UserActionsCounter::startCountingAll(QObject* target) {
    mTokens[target] = mEventCapturer->subscribeToEvents(
            target, [](const QObject* o) { return true; }, mEventTypes,
            [this](const QObject* target, const QEvent* event) {
                this->processEvent(target, event);
            });
    connect(target, SIGNAL(destroyed()), this, SLOT(stopCountingForEmitter()));
}

void UserActionsCounter::startCountingMarked(QObject* target) {
    mTokens[target] = mEventCapturer->subscribeToEvents(
            target, &UserActionsCounter::isMarked, mEventTypes,
            [this](const QObject* target, const QEvent* event) {
                this->processEvent(target, event);
            });
    connect(target, SIGNAL(destroyed()), this, SLOT(stopCountingForEmitter()));
}

void UserActionsCounter::stopCountingForEmitter() {
    if (auto target = sender()) {
        mTokens.erase(target);
    }
}

bool UserActionsCounter::isMarked(const QObject* target) {
    if (!target) {
        return false;
    }

    // We use the metrics reporting dynamic property to consider a widget as
    // worth counting. We'll count all user actions -- even for widgets where
    // no UI metrics are being reported otherwise.
    QVariant metricsEnabled = target->property(kReportMetricsProperty);

    // Dynamic properties are not inherited by child widgets, but the mouse
    // click events can be captured there in (e.g. QComboBox doesn't get any
    // QEvent when the selection is changed. A private sub-widget get the event
    // and translates that to QComboBox specific signals). So, also mark all
    // subwidgets of a marked widget.
    return metricsEnabled.isValid() || isMarked(target->parent());
}

void UserActionsCounter::processEvent(const QObject* target,
                                      const QEvent* event) {
    // All the filtering has already been done when registering for events.
    ++mCount;
}

}  // namespace qt
}  //  namespace android
