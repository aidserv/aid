/* Copyright (C) 2008 The Android Open Source Project
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
#include "android/avd/hw-config.h"
#include "android/utils/debug.h"
#include "android/utils/ini.h"
#include "android/utils/system.h"
#include <string.h>
#include <stdlib.h>


/* the global variable containing the hardware config for this device */
AndroidHwConfig   android_hw[1];

static int stringToBoolean(const char* value) {
    if (!strcmp(value,"1")    ||
        !strcmp(value,"yes")  ||
        !strcmp(value,"YES")  ||
        !strcmp(value,"true") ||
        !strcmp(value,"TRUE")) {
        return 1;
    }

    return 0;
}

static int stringToTribool(const char* value) {
    if (stringToBoolean(value)) {
        return 1;
    }

    // stringToBoolean() returns 0 for any non-"true" value. Let's see if it's
    // exact "false" or something in between.
    if (!strcmp(value,"0")    ||
        !strcmp(value,"no")  ||
        !strcmp(value,"NO")  ||
        !strcmp(value,"false") ||
        !strcmp(value,"FALSE")) {
        return 0;
    }
    return -1;
}

static int64_t
diskSizeToInt64( const char* diskSize )
{
    char*   end;
    int64_t value;

    value = strtoll(diskSize, &end, 10);
    if (*end == 'k' || *end == 'K')
        value *= 1024ULL;
    else if (*end == 'm' || *end == 'M')
        value *= 1024*1024ULL;
    else if (*end == 'g' || *end == 'G')
        value *= 1024*1024*1024ULL;

    return value;
}


void
androidHwConfig_init( AndroidHwConfig*  config,
                      int               apiLevel )
{
#define   HWCFG_BOOL(n,s,d,a,t)       config->n = stringToBoolean(d);
#define   HWCFG_INT(n,s,d,a,t)        config->n = d;
#define   HWCFG_STRING(n,s,d,a,t)     config->n = ASTRDUP(d);
#define   HWCFG_DOUBLE(n,s,d,a,t)     config->n = d;
#define   HWCFG_DISKSIZE(n,s,d,a,t)   config->n = diskSizeToInt64(d);

#include "android/avd/hw-config-defs.h"

    /* Special case for hw.keyboard.lid, we need to set the
     * default to FALSE for apiLevel >= 12. This allows platform builds
     * to get correct orientation emulation even if they don't bring
     * a custom hardware.ini
     */
    if (apiLevel >= 12) {
        config->hw_keyboard_lid = 0;
    }
}

int androidHwConfig_read(AndroidHwConfig* config, CIniFile* ini) {
    if (ini == NULL)
        return -1;

    /* use the magic of macros to implement the hardware configuration loaded */

#define   HWCFG_BOOL(n,s,d,a,t)       if (iniFile_hasKey(ini, s)) { config->n = iniFile_getBoolean(ini, s, d); }
#define   HWCFG_INT(n,s,d,a,t)        if (iniFile_hasKey(ini, s)) { config->n = iniFile_getInteger(ini, s, d); }
#define   HWCFG_STRING(n,s,d,a,t)     if (iniFile_hasKey(ini, s)) { AFREE(config->n); config->n = iniFile_getString(ini, s, d); }
#define   HWCFG_DOUBLE(n,s,d,a,t)     if (iniFile_hasKey(ini, s)) { config->n = iniFile_getDouble(ini, s, d); }
#define   HWCFG_DISKSIZE(n,s,d,a,t)   if (iniFile_hasKey(ini, s)) { config->n = iniFile_getDiskSize(ini, s, d); }

#include "android/avd/hw-config-defs.h"

    // Special case for the SD-Card, the AVD Manager can incorrectly create
    // a new AVD with 'sdcard.size=<size>' and 'hw.sdCard=no'. It's not sure
    // whether this occurs when updating an AVD settings or not, but it has
    // been described in:
    //       https://code.google.com/p/android/issues/detail?id=68429
    //
    // 'sdcard.size' is not a hardware property, so look it up directly in
    // the initFile() here and override a negative hw.sdCard value if it
    // is strictly positive.
    if (!config->hw_sdCard) {
        if (iniFile_getDiskSize(ini, "sdcard.size", "0") > 0) {
            VERBOSE_PRINT(init, "Overriding hw.sdCard to 'true' due to positive sdcard.size value!");
            config->hw_sdCard = true;
        }
    }
    return 0;
}

int androidHwConfig_write(AndroidHwConfig* config, CIniFile* ini) {
    if (ini == NULL)
        return -1;

    /* use the magic of macros to implement the hardware configuration loaded */

#define   HWCFG_BOOL(n,s,d,a,t)       iniFile_setBoolean(ini, s, config->n);
#define   HWCFG_INT(n,s,d,a,t)        iniFile_setInteger(ini, s, config->n);
#define   HWCFG_STRING(n,s,d,a,t)     iniFile_setValue(ini, s, config->n);
#define   HWCFG_DOUBLE(n,s,d,a,t)     iniFile_setDouble(ini, s, config->n);
#define   HWCFG_DISKSIZE(n,s,d,a,t)   iniFile_setDiskSize(ini, s, config->n);

#include "android/avd/hw-config-defs.h"

    return 0;
}

void
androidHwConfig_done( AndroidHwConfig* config )
{
#define   HWCFG_BOOL(n,s,d,a,t)       config->n = 0;
#define   HWCFG_INT(n,s,d,a,t)        config->n = 0;
#define   HWCFG_STRING(n,s,d,a,t)     AFREE(config->n);
#define   HWCFG_DOUBLE(n,s,d,a,t)     config->n = 0.0;
#define   HWCFG_DISKSIZE(n,s,d,a,t)   config->n = 0;

#include "android/avd/hw-config-defs.h"
}

int
androidHwConfig_isScreenNoTouch( AndroidHwConfig* config )
{
    return strcmp(config->hw_screen, "no-touch") == 0;
}

int
androidHwConfig_isScreenTouch( AndroidHwConfig* config )
{
    return strcmp(config->hw_screen, "touch") == 0;
}

int
androidHwConfig_isScreenMultiTouch( AndroidHwConfig* config )
{
    return strcmp(config->hw_screen, "multi-touch") == 0;
}

hwLcd_screenSize_t
androidHwConfig_getScreenSize( AndroidHwConfig* config )
{
    return hwLcd_getScreenSize(config->hw_lcd_height, config->hw_lcd_height,
                               config->hw_lcd_density);
}

int androidHwConfig_getMinVmHeapSize(AndroidHwConfig* config, int apiLevel) {
    int minVMHeapSize = 16;
    hwLcd_screenSize_t screenSize = androidHwConfig_getScreenSize(config);
    // Taken from requirements in CDD Documents on VM/Runtime Compatibility
    // TODO: android wear minimums
    if (apiLevel >= 23) {
        if (screenSize >= LCD_SIZE_XLARGE) {
            // TODO(zyy): emulator currently is unable to allocate a heap this
            // big, so it gets reduced to 576 in main-common.c
            if (config->hw_lcd_density >= LCD_DENSITY_XXXHDPI) {
                minVMHeapSize = 768;
            } else if (config->hw_lcd_density >= LCD_DENSITY_560DPI) {
                minVMHeapSize = 576;
            } else if (config->hw_lcd_density >= LCD_DENSITY_XXHDPI) {
                minVMHeapSize = 384;
            } else if (config->hw_lcd_density >= LCD_DENSITY_420DPI) {
                minVMHeapSize = 336;
            } else if (config->hw_lcd_density >= LCD_DENSITY_400DPI) {
                minVMHeapSize = 288;
            } else if (config->hw_lcd_density >= LCD_DENSITY_360DPI) {
                minVMHeapSize = 240;
            } else if (config->hw_lcd_density >= LCD_DENSITY_XHDPI) {
                minVMHeapSize = 192;
            } else if (config->hw_lcd_density >= LCD_DENSITY_280DPI) {
                minVMHeapSize = 144;
            } else if (config->hw_lcd_density >= LCD_DENSITY_HDPI) {
                minVMHeapSize = 96;
            } else if (config->hw_lcd_density >= LCD_DENSITY_TVDPI) {
                minVMHeapSize = 96;
            } else if (config->hw_lcd_density >= LCD_DENSITY_MDPI) {
                minVMHeapSize = 80;
            } else {
                minVMHeapSize = 48;
            }
        } else if (screenSize >= LCD_SIZE_LARGE) {
            if (config->hw_lcd_density >= LCD_DENSITY_XXXHDPI) {
                minVMHeapSize = 512;
            } else if (config->hw_lcd_density >= LCD_DENSITY_560DPI) {
                minVMHeapSize = 384;
            } else if (config->hw_lcd_density >= LCD_DENSITY_XXHDPI) {
                minVMHeapSize = 256;
            } else if (config->hw_lcd_density >= LCD_DENSITY_420DPI) {
                minVMHeapSize = 228;
            } else if (config->hw_lcd_density >= LCD_DENSITY_400DPI) {
                minVMHeapSize = 192;
            } else if (config->hw_lcd_density >= LCD_DENSITY_360DPI) {
                minVMHeapSize = 160;
            } else if (config->hw_lcd_density >= LCD_DENSITY_XHDPI) {
                minVMHeapSize = 128;
            } else if (config->hw_lcd_density >= LCD_DENSITY_280DPI) {
                minVMHeapSize = 96;
            } else if (config->hw_lcd_density >= LCD_DENSITY_HDPI) {
                minVMHeapSize = 80;
            } else if (config->hw_lcd_density >= LCD_DENSITY_TVDPI) {
                minVMHeapSize = 80;
            } else if (config->hw_lcd_density >= LCD_DENSITY_MDPI) {
                minVMHeapSize = 48;
            } else {
                minVMHeapSize = 32;
            }
        } else { // screenSize >= LCD_SIZE_SMALL
            if (config->hw_lcd_density >= LCD_DENSITY_XXXHDPI) {
                minVMHeapSize = 256;
            } else if (config->hw_lcd_density >= LCD_DENSITY_560DPI) {
                minVMHeapSize = 192;
            } else if (config->hw_lcd_density >= LCD_DENSITY_XXHDPI) {
                minVMHeapSize = 128;
            } else if (config->hw_lcd_density >= LCD_DENSITY_420DPI) {
                minVMHeapSize = 112;
            } else if (config->hw_lcd_density >= LCD_DENSITY_400DPI) {
                minVMHeapSize = 96;
            } else if (config->hw_lcd_density >= LCD_DENSITY_360DPI) {
                minVMHeapSize = 80;
            } else if (config->hw_lcd_density >= LCD_DENSITY_XHDPI) {
                minVMHeapSize = 80;
            } else if (config->hw_lcd_density >= LCD_DENSITY_280DPI) {
                minVMHeapSize = 48;
            } else if (config->hw_lcd_density >= LCD_DENSITY_HDPI) {
                minVMHeapSize = 48;
            } else if (config->hw_lcd_density >= LCD_DENSITY_TVDPI) {
                minVMHeapSize = 48;
            } else if (config->hw_lcd_density >= LCD_DENSITY_MDPI) {
                minVMHeapSize = 32;
            } else {
                minVMHeapSize = 32;
            }
        }
    } else if (apiLevel >= 22) {
        if (screenSize >= LCD_SIZE_XLARGE) {
            if (config->hw_lcd_density >= LCD_DENSITY_XXXHDPI) {
                minVMHeapSize = 768;
            } else if (config->hw_lcd_density >= LCD_DENSITY_560DPI) {
                minVMHeapSize = 576;
            } else if (config->hw_lcd_density >= LCD_DENSITY_XXHDPI) {
                minVMHeapSize = 384;
            } else if (config->hw_lcd_density >= LCD_DENSITY_400DPI) {
                minVMHeapSize = 288;
            } else if (config->hw_lcd_density >= LCD_DENSITY_XHDPI) {
                minVMHeapSize = 192;
            } else if (config->hw_lcd_density >= LCD_DENSITY_280DPI) {
                minVMHeapSize = 144;
            } else if (config->hw_lcd_density >= LCD_DENSITY_HDPI) {
                minVMHeapSize = 96;
            } else if (config->hw_lcd_density >= LCD_DENSITY_TVDPI) {
                minVMHeapSize = 96;
            } else if (config->hw_lcd_density >= LCD_DENSITY_MDPI) {
                minVMHeapSize = 80;
            } else {
                minVMHeapSize = 48;
            }
        } else if (screenSize >= LCD_SIZE_LARGE) {
            if (config->hw_lcd_density >= LCD_DENSITY_XXXHDPI) {
                minVMHeapSize = 512;
            } else if (config->hw_lcd_density >= LCD_DENSITY_560DPI) {
                minVMHeapSize = 384;
            } else if (config->hw_lcd_density >= LCD_DENSITY_XXHDPI) {
                minVMHeapSize = 256;
            } else if (config->hw_lcd_density >= LCD_DENSITY_400DPI) {
                minVMHeapSize = 192;
            } else if (config->hw_lcd_density >= LCD_DENSITY_XHDPI) {
                minVMHeapSize = 128;
            } else if (config->hw_lcd_density >= LCD_DENSITY_280DPI) {
                minVMHeapSize = 96;
            } else if (config->hw_lcd_density >= LCD_DENSITY_HDPI) {
                minVMHeapSize = 80;
            } else if (config->hw_lcd_density >= LCD_DENSITY_TVDPI) {
                minVMHeapSize = 80;
            } else if (config->hw_lcd_density >= LCD_DENSITY_MDPI) {
                minVMHeapSize = 48;
            } else {
                minVMHeapSize = 32;
            }
        } else { // screenSize >= LCD_SIZE_SMALL
            if (config->hw_lcd_density >= LCD_DENSITY_XXXHDPI) {
                minVMHeapSize = 256;
            } else if (config->hw_lcd_density >= LCD_DENSITY_560DPI) {
                minVMHeapSize = 192;
            } else if (config->hw_lcd_density >= LCD_DENSITY_XXHDPI) {
                minVMHeapSize = 128;
            } else if (config->hw_lcd_density >= LCD_DENSITY_400DPI) {
                minVMHeapSize = 96;
            } else if (config->hw_lcd_density >= LCD_DENSITY_XHDPI) {
                minVMHeapSize = 80;
            } else if (config->hw_lcd_density >= LCD_DENSITY_280DPI) {
                minVMHeapSize = 48;
            } else if (config->hw_lcd_density >= LCD_DENSITY_HDPI) {
                minVMHeapSize = 48;
            } else if (config->hw_lcd_density >= LCD_DENSITY_TVDPI) {
                minVMHeapSize = 48;
            } else if (config->hw_lcd_density >= LCD_DENSITY_MDPI) {
                minVMHeapSize = 32;
            } else {
                minVMHeapSize = 32;
            }
        }
    }

    else if (apiLevel >= 21) {
        if (screenSize >= LCD_SIZE_XLARGE) {
            if (config->hw_lcd_density >= LCD_DENSITY_XXXHDPI) {
                minVMHeapSize = 768;
            } else if (config->hw_lcd_density >= LCD_DENSITY_560DPI) {
                minVMHeapSize = 576;
            } else if (config->hw_lcd_density >= LCD_DENSITY_XXHDPI) {
                minVMHeapSize = 384;
            } else if (config->hw_lcd_density >= LCD_DENSITY_400DPI) {
                minVMHeapSize = 288;
            } else if (config->hw_lcd_density >= LCD_DENSITY_XHDPI) {
                minVMHeapSize = 192;
            } else if (config->hw_lcd_density >= LCD_DENSITY_HDPI) {
                minVMHeapSize = 96;
            } else if (config->hw_lcd_density >= LCD_DENSITY_TVDPI) {
                minVMHeapSize = 96;
            } else {
                minVMHeapSize = 64;
            }
        } else if (screenSize >= LCD_SIZE_LARGE) {
            if (config->hw_lcd_density >= LCD_DENSITY_XXXHDPI) {
                minVMHeapSize = 512;
            } else if (config->hw_lcd_density >= LCD_DENSITY_560DPI) {
                minVMHeapSize = 384;
            } else if (config->hw_lcd_density >= LCD_DENSITY_XXHDPI) {
                minVMHeapSize = 256;
            } else if (config->hw_lcd_density >= LCD_DENSITY_400DPI) {
                minVMHeapSize = 192;
            } else if (config->hw_lcd_density >= LCD_DENSITY_XHDPI) {
                minVMHeapSize = 128;
            } else if (config->hw_lcd_density >= LCD_DENSITY_HDPI) {
                minVMHeapSize = 64;
            } else if (config->hw_lcd_density >= LCD_DENSITY_TVDPI) {
                minVMHeapSize = 64;
            } else if (config->hw_lcd_density >= LCD_DENSITY_MDPI) {
                minVMHeapSize = 32;
            } else {
                minVMHeapSize = 16;
            }
        } else { // screenSize >= LCD_SIZE_SMALL
            if (config->hw_lcd_density >= LCD_DENSITY_XXXHDPI) {
                minVMHeapSize = 256;
            } else if (config->hw_lcd_density >= LCD_DENSITY_560DPI) {
                minVMHeapSize = 192;
            } else if (config->hw_lcd_density >= LCD_DENSITY_XXHDPI) {
                minVMHeapSize = 128;
            } else if (config->hw_lcd_density >= LCD_DENSITY_400DPI) {
                minVMHeapSize = 96;
            } else if (config->hw_lcd_density >= LCD_DENSITY_XHDPI) {
                minVMHeapSize = 64;
            } else if (config->hw_lcd_density >= LCD_DENSITY_HDPI) {
                minVMHeapSize = 32;
            } else if (config->hw_lcd_density >= LCD_DENSITY_TVDPI) {
                minVMHeapSize = 32;
            } else if (config->hw_lcd_density >= LCD_DENSITY_MDPI) {
                minVMHeapSize = 16;
            } else {
                minVMHeapSize = 16;
            }
        }
    } else if (apiLevel >= 19) {
        if (screenSize >= LCD_SIZE_XLARGE) {
            if (config->hw_lcd_density >= LCD_DENSITY_XXHDPI) {
                minVMHeapSize = 256;
            } else if (config->hw_lcd_density >= LCD_DENSITY_400DPI) {
                minVMHeapSize = 192;
            } else if (config->hw_lcd_density >= LCD_DENSITY_XHDPI) {
                minVMHeapSize = 128;
            } else if (config->hw_lcd_density >= LCD_DENSITY_HDPI) {
                minVMHeapSize = 64;
            } else if (config->hw_lcd_density >= LCD_DENSITY_TVDPI) {
                minVMHeapSize = 64;
            } else {
                minVMHeapSize = 32;
            }
        } else { // screenSize >= LCD_SIZE_SMALL
            if (config->hw_lcd_density >= LCD_DENSITY_XXHDPI) {
                minVMHeapSize = 128;
            } else if (config->hw_lcd_density >= LCD_DENSITY_400DPI) {
                minVMHeapSize = 96;
            } else if (config->hw_lcd_density >= LCD_DENSITY_XHDPI) {
                minVMHeapSize = 64;
            } else if (config->hw_lcd_density >= LCD_DENSITY_HDPI) {
                minVMHeapSize = 32;
            } else if (config->hw_lcd_density >= LCD_DENSITY_TVDPI) {
                minVMHeapSize = 32;
            } else if (config->hw_lcd_density >= LCD_DENSITY_MDPI) {
                minVMHeapSize = 16;
            } else {
                minVMHeapSize = 16;
            }
        }
    } else if (apiLevel >= 14) {
        if (screenSize >= LCD_SIZE_XLARGE) {
            if (config->hw_lcd_density >= LCD_DENSITY_XHDPI) {
                minVMHeapSize = 128;
            } else if (config->hw_lcd_density >= LCD_DENSITY_HDPI) {
                minVMHeapSize = 64;
            } else if (config->hw_lcd_density >= LCD_DENSITY_TVDPI) {
                minVMHeapSize = 64;
            } else {
                minVMHeapSize = 32;
            }
        } else { // screenSize >= LCD_SIZE_SMALL
            if (config->hw_lcd_density >= LCD_DENSITY_XHDPI) {
                minVMHeapSize = 64;
            } else if (config->hw_lcd_density >= LCD_DENSITY_HDPI) {
                minVMHeapSize = 32;
            } else if (config->hw_lcd_density >= LCD_DENSITY_TVDPI) {
                minVMHeapSize = 32;
            } else if (config->hw_lcd_density >= LCD_DENSITY_MDPI) {
                minVMHeapSize = 16;
            } else {
                minVMHeapSize = 16;
            }
        }
    } else if (apiLevel >= 7) {
        if (config->hw_lcd_density >= 240) {
            minVMHeapSize = 24;
        } else {
            minVMHeapSize = 16;
        }
    } else {
        minVMHeapSize = 16;
    }
    return minVMHeapSize;
}

int androidHwConfig_getKernelDeviceNaming(AndroidHwConfig* config) {
    return stringToTribool(config->kernel_newDeviceNaming);
}

int androidHwConfig_getKernelYaffs2Support(AndroidHwConfig* config) {
    return stringToTribool(config->kernel_supportsYaffs2);
}

const char* androidHwConfig_getKernelSerialPrefix(AndroidHwConfig* config )
{
    if (androidHwConfig_getKernelDeviceNaming(config) >= 1) {
        return "ttyGF";
    } else {
        return "ttyS";
    }
}
