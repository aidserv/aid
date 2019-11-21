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

#include "android/emulation/control/callbacks.h"
#include "android/utils/compiler.h"

#include <stdbool.h>

ANDROID_BEGIN_HEADER

enum BatteryCharger {
    BATTERY_CHARGER_NONE,
    BATTERY_CHARGER_AC,
    // TODO: The UI only allows NONE and AC.
    //       Need to also allow and implement USB and WIRELESS.
    BATTERY_CHARGER_USB,
    BATTERY_CHARGER_WIRELESS,
    BATTERY_CHARGER_NUM_ENTRIES
};

enum BatteryHealth {
    BATTERY_HEALTH_GOOD,
    BATTERY_HEALTH_FAILED,
    BATTERY_HEALTH_DEAD,
    BATTERY_HEALTH_OVERVOLTAGE,
    BATTERY_HEALTH_OVERHEATED,
    BATTERY_HEALTH_UNKNOWN,
    BATTERY_HEALTH_NUM_ENTRIES
};

enum BatteryStatus {
    BATTERY_STATUS_UNKNOWN,
    BATTERY_STATUS_CHARGING,
    BATTERY_STATUS_DISCHARGING,
    BATTERY_STATUS_NOT_CHARGING,
    BATTERY_STATUS_FULL,
    BATTERY_STATUS_NUM_ENTRIES
};

typedef struct QAndroidBatteryAgent {
    // Sets whether the battery is present or not
    void (*setIsBatteryPresent)(bool isPresent);

    // Returns whether the battery is present or not
    bool (*present)();

    // Sets whether the battery is charging or not
    void (*setIsCharging)(bool isCharging);

    // Returns whether the battery is charging or not
    bool (*charging)();

    // Set what type of charger is being used
    void (*setCharger)(enum BatteryCharger charger);

    // Returns what type of charger is being used
    enum BatteryCharger (*charger)();

    // Sets the current charge level
    // Input: |percentFull| integer percentage, 0 .. 100
    void (*setChargeLevel)(int percentFull);

    // Returns the current charge level as a
    // percentage, 0 .. 100
    int (*chargeLevel)();

    // Sets the battery health
    // Input: |health| one of the BatteryHealth enum values, above
    void (*setHealth)(enum BatteryHealth health);

    // Returns the battery health as one of the BatteryHealth
    // enum values, above
    enum BatteryHealth (*health)();

    // Sets the battery status
    // Input: |status| one of the BatteryStatus enum values, above
    void (*setStatus)(enum BatteryStatus status);

    // Returns the battery status as one of the BatteryStatus
    // enum values, above
    enum BatteryStatus (*status)();

    // Query the battery state
    void (*batteryDisplay)(void* opaque, LineConsumerCallback outputCallback);
} QAndroidBatteryAgent;

ANDROID_END_HEADER
