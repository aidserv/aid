/* Copyright (C) 2015 The Android Open Source Project
 **
 ** This software is licensed under the terms of the GNU General Public
 ** License version 2, as published by the Free Software Foundation, and
 ** may be copied, distributed, and modified under those terms.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 */

#pragma once

// This file contains the colors and other styles that determine
// how the extended window appears

// This gives the order of the tabbed panes on the extended window.
// This must correspond to the ordering that is set from within
// QtDesigner and written to extended.ui.
enum ExtendedWindowPane {
    PANE_IDX_LOCATION = 0,
    PANE_IDX_CELLULAR,
    PANE_IDX_BATTERY,
    PANE_IDX_TELEPHONE,
    PANE_IDX_VIRT_SENSORS,
    PANE_IDX_DPAD,
    PANE_IDX_ROTARY,
    PANE_IDX_MICROPHONE,
    PANE_IDX_FINGER,
    PANE_IDX_RECORD_SCREEN,
    PANE_IDX_GOOGLE_PLAY,
    PANE_IDX_SETTINGS,
    PANE_IDX_HELP,
    PANE_IDX_CAR,
};
