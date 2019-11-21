/* Copyright (C) 2007-2008 The Android Open Source Project
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

#include "android/skin/rect.h"
#include "android/skin/surface.h"

typedef struct SkinTrackBall  SkinTrackBall;

typedef void (*SkinTrackBallEventFunc)(int dx, int dy);

typedef struct SkinTrackBallParameters
{
    int       diameter;
    int       ring;
    unsigned  ball_color;
    unsigned  dot_color;
    unsigned  ring_color;

    SkinTrackBallEventFunc event_func;
}
SkinTrackBallParameters;


extern SkinTrackBall*  skin_trackball_create(
        const SkinTrackBallParameters*  params);

extern void skin_trackball_rect(SkinTrackBall* ball, SkinRect* rect);

extern int skin_trackball_move(SkinTrackBall* ball, int dx, int dy);

extern void skin_trackball_refresh(SkinTrackBall*  ball);

extern void skin_trackball_draw(SkinTrackBall* ball,
                                int x,
                                int y,
                                SkinSurface* dst);

extern void skin_trackball_destroy (SkinTrackBall*  ball);

/* this sets the rotation that will be applied to mouse events sent to the system */
extern void skin_trackball_set_rotation(SkinTrackBall*  ball, SkinRotation  rotation);
