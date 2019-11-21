// Copyright (C) 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include <gtest/gtest.h>

#include "android/hw-lcd.h"

namespace lcd_density {

TEST(Hw_lcd, MapDensity) {

    EXPECT_EQ(LCD_DENSITY_TVDPI,   hwLcd_mapDensity(LCD_DENSITY_TVDPI));

    EXPECT_EQ(LCD_DENSITY_LDPI,    hwLcd_mapDensity(110));
    EXPECT_EQ(LCD_DENSITY_260DPI,  hwLcd_mapDensity(266));
    EXPECT_EQ(LCD_DENSITY_280DPI,  hwLcd_mapDensity(272));
    EXPECT_EQ(LCD_DENSITY_560DPI,  hwLcd_mapDensity(590));
    EXPECT_EQ(LCD_DENSITY_XXXHDPI, hwLcd_mapDensity(610));
    EXPECT_EQ(LCD_DENSITY_XXXHDPI, hwLcd_mapDensity(999));
}

TEST(Hw_lcd, GetScreenSize) {

    // Made-up device (3.4 inches)
    EXPECT_EQ(LCD_SIZE_SMALL,  hwLcd_getScreenSize( 240,  480, LCD_DENSITY_MDPI));

    // Nexus 5 (4.95 inches)
    EXPECT_EQ(LCD_SIZE_NORMAL, hwLcd_getScreenSize(1080, 1920, LCD_DENSITY_XXHDPI));

    // Nexus 5x (5.2 inches)
    EXPECT_EQ(LCD_SIZE_LARGE,  hwLcd_getScreenSize(1080, 1920, LCD_DENSITY_420DPI));

    // Nexus 9 (8.86 inches)
    EXPECT_EQ(LCD_SIZE_XLARGE, hwLcd_getScreenSize(2048, 1536, LCD_DENSITY_XHDPI));
}

}  // namespace density
