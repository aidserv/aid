/* Copyright (C) 2009 The Android Open Source Project
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

#define  LCD_DENSITY_LDPI      120
#define  LCD_DENSITY_MDPI      160
#define  LCD_DENSITY_TVDPI     213
#define  LCD_DENSITY_HDPI      240
#define  LCD_DENSITY_260DPI    260
#define  LCD_DENSITY_280DPI    280
#define  LCD_DENSITY_300DPI    300
#define  LCD_DENSITY_XHDPI     320
#define  LCD_DENSITY_340DPI    340
#define  LCD_DENSITY_360DPI    360
#define  LCD_DENSITY_400DPI    400
#define  LCD_DENSITY_420DPI    420
#define  LCD_DENSITY_XXHDPI    480
#define  LCD_DENSITY_560DPI    560
#define  LCD_DENSITY_XXXHDPI   640

typedef enum hwLcd_screenSize {
    LCD_SIZE_SMALL,
    LCD_SIZE_NORMAL,
    LCD_SIZE_LARGE,
    LCD_SIZE_XLARGE
} hwLcd_screenSize_t;

ANDROID_BEGIN_HEADER

/* Sets the boot property corresponding to the emulated abstract LCD density */
extern void hwLcd_setBootProperty(int density);

extern hwLcd_screenSize_t hwLcd_getScreenSize(int heightPx,
                                              int widthPx,
                                              int density);

/* Don't call this directly.
 * It is public only to allow unit testing.
 */
extern int hwLcd_mapDensity(int density);

ANDROID_END_HEADER
