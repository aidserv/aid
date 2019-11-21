/* Copyright (C) 2014 The Android Open Source Project
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

#include <stdbool.h>
#include <stdint.h>

#include "android/skin/rect.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    kEventKeyDown,
    kEventKeyUp,
    kEventTextInput,
    kEventMouseButtonDown,
    kEventMouseButtonUp,
    kEventMouseMotion,
    kEventQuit,
    kEventScrollBarChanged,
    kEventRotaryInput,
    kEventSetScale,
    kEventSetZoom,
    kEventForceRedraw,
    kEventWindowMoved,
    kEventLayoutRotate,
    kEventScreenChanged,
    kEventZoomedWindowResized,
    kEventToggleTrackball,
} SkinEventType;

typedef enum {
    kMouseButtonLeft,
    kMouseButtonSecondaryTouch,
    kMouseButtonRight
} SkinMouseButtonType;

typedef struct {
    uint32_t keycode;
    uint32_t mod;
} SkinEventKeyData;

typedef struct {
    uint8_t text[32];
    bool down;
} SkinEventTextInputData;

typedef struct {
    int x;
    int y;
    int xrel;
    int yrel;
    int button;
    int skip_sync;
} SkinEventMouseData;

typedef struct {
    SkinRotation rotation;
} SkinEventLayoutRotateData;

typedef struct {
    int x; // Send current window coordinates to maintain window location
    int y;
    int scroll_h; // Height of the horizontal scrollbar, needed for OSX
    double scale;
} SkinEventWindowData;

typedef struct {
    int x;
    int y;
    int xmax;
    int ymax;
    int scroll_h; // Height of the horizontal scrollbar, needed for OSX
} SkinEventScrollData;

typedef struct {
    int delta; // Positive or negative relative rotation
} SkinEventRotaryInputData;

typedef struct {
    SkinEventType type;
    union {
        SkinEventKeyData key;
        SkinEventMouseData mouse;
        SkinEventScrollData scroll;
        SkinEventRotaryInputData rotary_input;
        SkinEventTextInputData text;
        SkinEventWindowData window;
        SkinEventLayoutRotateData layout_rotation;
    } u;
} SkinEvent;

// Poll for incoming input events. On success, return true and sets |*event|.
// On failure, i.e. if there are no events, return false.
extern bool skin_event_poll(SkinEvent* event);

#ifdef __cplusplus
}
#endif
