// Copyright (C) 2015-2017 The Android Open Source Project
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include <QString>

enum class QtUICommand {
    SHOW_PANE_LOCATION,
    SHOW_PANE_CELLULAR,
    SHOW_PANE_BATTERY,
    SHOW_PANE_PHONE,
    SHOW_PANE_MICROPHONE,
    SHOW_PANE_VIRTSENSORS,
    SHOW_PANE_DPAD,
    SHOW_PANE_FINGER,
    SHOW_PANE_GPLAY,
    SHOW_PANE_SETTINGS,
    SHOW_PANE_HELP,
    SHOW_MULTITOUCH,
    TAKE_SCREENSHOT,
    ENTER_ZOOM,
    ZOOM_IN,
    ZOOM_OUT,
    PAN_UP,
    PAN_DOWN,
    PAN_LEFT,
    PAN_RIGHT,
    VOLUME_UP,
    VOLUME_DOWN,
    POWER,
    MENU,
    HOME,
    BACK,
    OVERVIEW,
    ROTATE_RIGHT,
    ROTATE_LEFT,
    TOGGLE_TRACKBALL,
};

bool parseQtUICommand(const QString& string, QtUICommand* command);
QString getQtUICommandDescription(QtUICommand command);
