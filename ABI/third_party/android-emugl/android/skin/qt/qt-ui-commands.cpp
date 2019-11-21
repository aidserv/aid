// Copyright (C) 2015-2017 The Android Open Source Project

// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/qt-ui-commands.h"

#include <QApplication>
#include <algorithm>

namespace {

struct CommandInfo {
    QtUICommand command;
    const char* asString;
    const char* description;
};

}  // namespace

#define INIT_COMMAND(cmd, desc) { QtUICommand::cmd, #cmd, desc }
constexpr CommandInfo kCommands[] = {
        INIT_COMMAND(SHOW_PANE_LOCATION, "Location"),
        INIT_COMMAND(SHOW_PANE_CELLULAR, "Cellular"),
        INIT_COMMAND(SHOW_PANE_BATTERY, "Battery"),
        INIT_COMMAND(SHOW_PANE_PHONE, "Phone"),
        INIT_COMMAND(SHOW_PANE_MICROPHONE, "Microphone"),
        INIT_COMMAND(SHOW_PANE_VIRTSENSORS, "Virtual sensors"),
        INIT_COMMAND(SHOW_PANE_DPAD, "D-Pad"),
        INIT_COMMAND(SHOW_PANE_FINGER, "Fingerprint"),
        INIT_COMMAND(SHOW_PANE_GPLAY, "Google Play"),
        INIT_COMMAND(SHOW_PANE_SETTINGS, "Settings"),
        INIT_COMMAND(SHOW_PANE_HELP, "Help"),
        INIT_COMMAND(SHOW_MULTITOUCH,
                     "Multitouch (left click to pinch/zoom, right click to "
                     "vertical swipe)"),
        INIT_COMMAND(ROTATE_LEFT, "Rotate left"),
        INIT_COMMAND(ROTATE_RIGHT, "Rotate right"),
        INIT_COMMAND(TAKE_SCREENSHOT, "Take screenshot"),
        INIT_COMMAND(ENTER_ZOOM, "Enter zoom mode"),
        INIT_COMMAND(ZOOM_IN,
                     "Zoom in (when in zoom mode), or scale up (when not)"),
        INIT_COMMAND(ZOOM_OUT,
                     "Zoom out (when in zoom mode), or scale down (when not)"),
        INIT_COMMAND(PAN_UP, "Pan up (when in zoom mode)"),
        INIT_COMMAND(PAN_DOWN, "Pan down (when in zoom mode)"),
        INIT_COMMAND(PAN_LEFT, "Pan left (when in zoom mode)"),
        INIT_COMMAND(PAN_RIGHT, "Pan right (when in zoom mode)"),
        INIT_COMMAND(VOLUME_UP, "Volume up"),
        INIT_COMMAND(VOLUME_DOWN, "Volume down"),
        INIT_COMMAND(POWER, "Power"),
        INIT_COMMAND(HOME, "Home"),
        INIT_COMMAND(BACK, "Back"),
        INIT_COMMAND(MENU, "Menu"),
        INIT_COMMAND(OVERVIEW, "Overview"),
        INIT_COMMAND(TOGGLE_TRACKBALL, "Toggle trackball mode (if AVD is configured for it)"),
};

bool parseQtUICommand(const QString& string, QtUICommand* command) {
    auto it = std::find_if(std::begin(kCommands), std::end(kCommands),
                           [&string](const CommandInfo& info) {
                               return info.asString == string;
                           });
    bool result = (it != std::end(kCommands));
    if (result) {
        *command = it->command;
    }
    return result;
}

QString getQtUICommandDescription(QtUICommand command) {
    QString result;
    auto it = std::find_if(std::begin(kCommands), std::end(kCommands),
                           [command](const CommandInfo& info) {
                               return info.command == command;
                           });
    if (it != std::end(kCommands)) {
        result = qApp->translate("QtUICommand", it->description);
    }

    return result;
}
