// Copyright 2016 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "android/skin/qt/event-serializer.h"

#include <QEnterEvent>
#include <QEvent>
#include <QMouseEvent>
#include <QResizeEvent>

#include <cassert>

std::ostream& operator<<(std::ostream& out, const QEvent& event) {
    switch (event.type()) {
    case QEvent::Enter:
        {
            const auto enter_event = static_cast<const QEnterEvent*>(&event);
            out << enter_event->localPos().x() << ' ' << enter_event->localPos().y() << ' '
                << enter_event->windowPos().x() << ' ' << enter_event->windowPos().y() << ' '
                << enter_event->screenPos().x() << ' ' << enter_event->screenPos().y();
            break;
        }
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
        {
            const auto mouse_event = static_cast<const QMouseEvent*>(&event);
            assert(mouse_event);
            out << mouse_event->localPos().x() << ' ' << mouse_event->localPos().y() << ' '
                << mouse_event->windowPos().x() << ' ' << mouse_event->windowPos().y() << ' '
                << mouse_event->screenPos().x() << ' ' << mouse_event->screenPos().y() << ' '
                << mouse_event->buttons() << ' '
                << mouse_event->modifiers();
            break;
        }
    case QEvent::Resize:
        {
            const auto resize_event = static_cast<const QResizeEvent*>(&event);
            assert(resize_event);
            out << resize_event->size().width() << ' ' << resize_event->size().height() << ' '
                << resize_event->oldSize().width() << ' ' << resize_event->oldSize().height();
            break;
        }
    case QEvent::Close:
    case QEvent::Leave:
    case QEvent::FocusIn:
    case QEvent::FocusOut:
    case QEvent::Hide:
        break;
    default:
        out << "<unsupported by serializer>";
    }

    return out;
}
