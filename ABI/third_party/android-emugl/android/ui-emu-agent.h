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

// Agent iterfaces for sending commands from the UI to the emulator

typedef struct UiEmuAgent {
    const struct QAndroidBatteryAgent* battery;
    const struct QAndroidCellularAgent* cellular;
    const struct QAndroidClipboardAgent* clipboard;
    const struct QAndroidEmulatorWindowAgent* window;
    const struct QAndroidFingerAgent* finger;
    const struct QAndroidLocationAgent* location;
    const struct QAndroidHttpProxyAgent* proxy;
    const struct QAndroidSensorsAgent* sensors;
    const struct QAndroidTelephonyAgent* telephony;
    const struct QAndroidUserEventAgent* userEvents;
    const struct QCarDataAgent* car;
    const struct SettingsAgent* settings;
} UiEmuAgent;
