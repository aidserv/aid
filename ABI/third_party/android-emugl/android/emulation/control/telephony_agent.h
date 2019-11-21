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

#include "android/telephony/modem.h"
#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

typedef enum {
    Tel_Op_Init_Call,
    Tel_Op_Accept_Call,
    Tel_Op_Reject_Call_Explicit,
    Tel_Op_Reject_Call_Busy,
    Tel_Op_Disconnect_Call,
    Tel_Op_Place_Call_On_Hold,
    Tel_Op_Take_Call_Off_Hold
} TelephonyOperation;

typedef enum {
    Tel_Resp_OK,
    Tel_Resp_Bad_Operation,  // Enum out of range
    Tel_Resp_Bad_Number,     // Mal-formed telephone number
    Tel_Resp_Invalid_Action, // E.g., disconnect when no call is in progress
    Tel_Resp_Action_Failed   // Internal error
} TelephonyResponse;

typedef struct QAndroidTelephonyAgent {
    TelephonyResponse (*telephonyCmd)(TelephonyOperation op, const char *phoneNumber);
    void (*initModem)(int basePort);
    AModem (*getModem)(void);
    void (*setNotifyCallback)(ModemCallback callbackFunc, void* userData);
} QAndroidTelephonyAgent;

ANDROID_END_HEADER
