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
#include "android/hw-lcd.h"
#include "android/boot-properties.h"
#include <math.h>
#include <stdio.h>

int
hwLcd_mapDensity(int density) {
    /* Map density to one of our bucket values.
       The TV density is a bit particular (and not actually a bucket
       value) so we do only exact match on it.
    */
    if (density != LCD_DENSITY_TVDPI) {
        if (density < (LCD_DENSITY_LDPI + LCD_DENSITY_MDPI)/2)
            density = LCD_DENSITY_LDPI;
        else if (density < (LCD_DENSITY_MDPI + LCD_DENSITY_HDPI)/2)
            density = LCD_DENSITY_MDPI;
        else if (density < (LCD_DENSITY_HDPI + LCD_DENSITY_260DPI)/2)
            density = LCD_DENSITY_HDPI;
        else if (density < (LCD_DENSITY_260DPI + LCD_DENSITY_280DPI)/2)
            density = LCD_DENSITY_260DPI;
        else if (density < (LCD_DENSITY_280DPI + LCD_DENSITY_300DPI)/2)
            density = LCD_DENSITY_280DPI;
        else if (density < (LCD_DENSITY_300DPI + LCD_DENSITY_XHDPI)/2)
            density = LCD_DENSITY_300DPI;
        else if (density < (LCD_DENSITY_XHDPI + LCD_DENSITY_340DPI)/2)
            density = LCD_DENSITY_XHDPI;
        else if (density < (LCD_DENSITY_340DPI + LCD_DENSITY_360DPI)/2)
            density = LCD_DENSITY_340DPI;
        else if (density < (LCD_DENSITY_360DPI + LCD_DENSITY_400DPI)/2)
            density = LCD_DENSITY_360DPI;
        else if (density < (LCD_DENSITY_400DPI + LCD_DENSITY_420DPI) / 2)
            density = LCD_DENSITY_400DPI;
        else if (density < (LCD_DENSITY_420DPI + LCD_DENSITY_XXHDPI) / 2)
            density = LCD_DENSITY_420DPI;
        else if (density < (LCD_DENSITY_XXHDPI + LCD_DENSITY_560DPI)/2)
            density = LCD_DENSITY_XXHDPI;
        else if (density < (LCD_DENSITY_560DPI + LCD_DENSITY_XXXHDPI)/2)
            density = LCD_DENSITY_560DPI;
        else
            density = LCD_DENSITY_XXXHDPI;
    }
    return density;
}


void
hwLcd_setBootProperty(int density)
{
    char  temp[8];
    int mapped_density;

    mapped_density = hwLcd_mapDensity(density);


    snprintf(temp, sizeof temp, "%d", mapped_density);
    boot_property_add("qemu.sf.lcd_density", temp);
}

hwLcd_screenSize_t hwLcd_getScreenSize(int heightPx, int widthPx, int density) {
    double screen_inches = sqrt(pow((double)heightPx / density, 2) +
                               pow((double)widthPx / density, 2));

    if (screen_inches >= 7.5) {
        return LCD_SIZE_XLARGE;
    }
    if (screen_inches >= 5) {
        return LCD_SIZE_LARGE;
    }
    if (screen_inches >= 3.6) {
        return LCD_SIZE_NORMAL;
    }
    return LCD_SIZE_SMALL;
}
