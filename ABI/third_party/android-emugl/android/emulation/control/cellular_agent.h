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

#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

enum CellularStatus { Cellular_Stat_Home,   Cellular_Stat_Roaming, Cellular_Stat_Searching,
                      Cellular_Stat_Denied, Cellular_Stat_Unregistered };

enum CellularStandard { Cellular_Std_GSM,  Cellular_Std_HSCSD, Cellular_Std_GPRS, Cellular_Std_EDGE,
                        Cellular_Std_UMTS, Cellular_Std_HSDPA, Cellular_Std_LTE, Cellular_Std_full };

enum CellularSignal { Cellular_Signal_None, Cellular_Signal_Poor, Cellular_Signal_Moderate,
                      Cellular_Signal_Good, Cellular_Signal_Great };

typedef struct QAndroidCellularAgent {
    // Sets the cellular signal strength
    // Input: 0(none) .. 31(very strong)
    void (*setSignalStrength)(int zeroTo31);

    // Sets the cellular signal strength
    // Input: enum CellularSignal, above
    void (*setSignalStrengthProfile)(enum CellularSignal);

    // Sets the status of the voice connectivity
    // Input: enum CellularStatus, above
    void (*setVoiceStatus)(enum CellularStatus);

    // Sets the status of the data connectivity
    // Input: enum CellularStatus, above
    void (*setDataStatus)(enum CellularStatus);

    // Sets the cellular data standard in use
    // Input: enum CellularStandard, above
    void (*setStandard)(enum CellularStandard);
} QAndroidCellularAgent;

ANDROID_END_HEADER
