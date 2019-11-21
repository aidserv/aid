// Copyright 2015 The Android Open Source Project
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

#pragma once

#include "android/utils/compiler.h"

#include <stdbool.h>

ANDROID_BEGIN_HEADER

// C interface to expose Qemu implementation of user event piping to the VM.
typedef struct QAndroidUserEventAgent {
    // Send various input user events to the VM.
    // Note that keycodes are all non-negative. But the world is a dreary place
    // and everyone accepts int for keycodes, so we do as the Romans do.
    void (*sendKey)(unsigned key, bool down);
    void (*sendKeyCode)(int key);
    void (*sendKeyCodes)(int* keycodes, int count);

    // Mouse event.
    void (*sendMouseEvent)(int dx, int dy, int dz, int buttonsState);

    // Rotary encoder events
    // delta is a positive or negative value indicating the change of angle
    // for the rotary input, in degrees. Positive values correspond to
    // counterclockwise rotation.
    void (*sendRotaryEvent)(int delta);

    // Send generic input events.
    void (*sendGenericEvent)(int type, int code, int value);

    // notify the emulator that new user event is available
    void (*onNewUserEvent)(void);
} QAndroidUserEventAgent;

ANDROID_END_HEADER
