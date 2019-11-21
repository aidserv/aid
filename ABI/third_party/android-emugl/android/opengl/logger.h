// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

// The purpose of the OpenGL logger is to log
// information about such things as EGL initialization
// and possibly miscellanous OpenGL commands,
// in order to get a better idea of what is going on
// in crash reports.

// C interface for android-emugl
void android_init_opengl_logger();
void android_opengl_logger_write(const char* fmt, ...);
void android_stop_opengl_logger();

// This is for logging what goes on in individual OpenGL
// contexts (cxts). Only called when emugl is compiled
// with -DOPENGL_DEBUG_PRINTOUT.
void android_opengl_cxt_logger_write(const char* fmt, ...);

ANDROID_END_HEADER
