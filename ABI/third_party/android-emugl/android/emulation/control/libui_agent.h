// Copyright (C) 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 

#pragma once

#include "android/utils/compiler.h"

#include <stdbool.h>

ANDROID_BEGIN_HEADER

typedef void (*LibuiKeyCodeSendFunc)(int* codes, int count, void* context);

typedef struct QAndroidLibuiAgent {
    // Converts the passed text into a set of UI input events and sends them
    // using |sendFunc|.
    bool (*convertUtf8ToKeyCodeEvents)(
            const unsigned char* text, int len, LibuiKeyCodeSendFunc sendFunc);

    // Requests UI code to gracefully shut down and exit. Doesn't wait for it to
    // complete.
    void (*requestExit)(int exitCode, const char* message);
} QAndroidLibuiAgent;

ANDROID_END_HEADER
