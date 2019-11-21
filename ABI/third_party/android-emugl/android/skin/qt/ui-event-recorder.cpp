// Copyright 2017 The Android Open Source Project
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/ui-event-recorder.h"

#include "android/base/memory/LazyInstance.h"
#include "android/skin/qt/event-serializer.h"

#include <QEvent>
#include <QMetaEnum>
#include <iomanip>

namespace {

struct Globals {
    int eventEnumIndex = QEvent::staticMetaObject.indexOfEnumerator("Type");

    EventCapturer::EventTypeSet events = {QEvent::Close,
                                          QEvent::Enter,
                                          QEvent::Leave,
                                          QEvent::FocusIn,
                                          QEvent::FocusOut,
                                          QEvent::Hide,
                                          QEvent::MouseButtonPress,
                                          QEvent::MouseButtonRelease,
                                          QEvent::Resize};
};

static android::base::LazyInstance<Globals> sGlobals = {};

}  // namespace

std::ostream& operator<<(std::ostream& out,
                         const UIEventRecorderBase::EventRecord& event) {
    out << '[' << std::setw(10) << event.uptimeMs << "] " << std::setw(20)
        << (event.targetName.empty() ? "<none>" : event.targetName) << ' '
        << std::setw(20)
        << QEvent::staticMetaObject.enumerator(sGlobals->eventEnumIndex)
                    .valueToKey(event.event->type())
        << " :: {" << *event.event << '}';
    return out;
}

const EventCapturer::EventTypeSet& UIEventRecorderBase::eventTypes() const {
    return sGlobals->events;
}
