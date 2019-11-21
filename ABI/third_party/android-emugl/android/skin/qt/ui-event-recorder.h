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

#include "android/base/system/System.h"
#include "android/skin/qt/event-subscriber.h"

#include <QEvent>

#include <sstream>
#include <string>
#include <utility>

// Base non-template class so eventTypes() can be implemented in .cpp
class UIEventRecorderBase {
protected:
    ~UIEventRecorderBase() = default;
    const EventCapturer::EventTypeSet& eventTypes() const;

public:
    struct EventRecord {
        android::base::System::WallDuration uptimeMs;
        std::string targetName;
        const QEvent* event;
    };
};

std::ostream& operator<<(std::ostream& out,
                         const UIEventRecorderBase::EventRecord& event);

// This class intercepts various UI events via EventCapturer
// and stores their serialized representations in a container.
template <template <class T, class A = std::allocator<T>> class EventContainer>
class UIEventRecorder : public EventSubscriber, private UIEventRecorderBase {
public:
    using ContainerType = EventContainer<std::string>;

    template <typename... Args>
    explicit UIEventRecorder(EventCapturer* ecap, Args&&... args)
        : EventSubscriber(ecap), mContainer(std::forward<Args>(args)...) {}

    // Get a const reference to the underelying container with serialized
    // events.
    const ContainerType& container() const { return mContainer; }

    void stop() { mRecording = false; }

private:
    bool objectPredicate(const QObject*) const override { return mRecording; }

    const EventCapturer::EventTypeSet& eventTypes() const override {
        return UIEventRecorderBase::eventTypes();
    }

    void processEvent(const QObject* target, const QEvent* event) override {
        EventRecord record{
                android::base::System::get()->getProcessTimes().wallClockMs,
                target->objectName().toStdString(), event};

        std::ostringstream str;
        str << record;
        mContainer.push_back(str.str());
    }

private:
    ContainerType mContainer;
    bool mRecording = true;
};
