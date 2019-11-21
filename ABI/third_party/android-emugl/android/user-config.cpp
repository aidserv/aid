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
#include "android/user-config.h"

#include "android/base/memory/ScopedPtr.h"

#include "android/emulation/bufprint_config_dirs.h"
#include "android/utils/bufprint.h"
#include "android/utils/debug.h"
#include "android/utils/system.h"
#include "android/utils/path.h"
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>

#define  D(...)   VERBOSE_PRINT(init,__VA_ARGS__)

#if 0 /* set to 1 for more debugging */
#  define  DD(...)  D(__VA_ARGS__)
#else
#  define  DD(...)  ((void)0)
#endif

struct AUserConfig {
    ABool        changed;
    int          windowX;
    int          windowY;
    uint64_t     uuid;
    char*        iniPath;
};

/* Name of the user-config file */
#define  USER_CONFIG_FILE  "emulator-user.ini"

#define  KEY_WINDOW_X  "window.x"
#define  KEY_WINDOW_Y  "window.y"
#define  KEY_UUID      "uuid"

#define  DEFAULT_X  100
#define  DEFAULT_Y  100

void auserConfig_free( AUserConfig* uconfig) {
    if (uconfig->iniPath) {
        free(uconfig->iniPath);
    }
    delete uconfig;
}

/* Create a new AUserConfig object from a given AvdInfo */
AUserConfig*
auserConfig_new( AvdInfo*  info )
{
    char          inAndroidBuild = avdInfo_inAndroidBuild(info);
    char          needUUID = 1;
    char          temp[PATH_MAX], *p=temp, *end=p+sizeof(temp);
    CIniFile*     ini = NULL;

    std::unique_ptr<AUserConfig> uc(new AUserConfig());
    memset(uc.get(), 0, sizeof(*uc));

    /* If we are in the Android build system, store the configuration
     * in ~/.android/emulator-user.ini. otherwise, store it in the file
     * emulator-user.ini in the AVD's content directory.
     */
    if (inAndroidBuild) {
        p = bufprint_config_file(temp, end, USER_CONFIG_FILE);
    } else {
        p = bufprint(temp, end, "%s/%s", avdInfo_getContentPath(info),
                     USER_CONFIG_FILE);
    }

    /* handle the unexpected */
    if (p >= end) {
        /* Hmmm, something is weird, let's use a temporary file instead */
        p = bufprint_temp_file(temp, end, USER_CONFIG_FILE);
        if (p >= end) {
            derror("Weird: Cannot create temporary user-config file?");
            return NULL;
        }
        dwarning("Weird: Content path too long, using temporary user-config.");
    }

    uc->iniPath = ASTRDUP(temp);
    DD("looking user-config in: %s", uc->iniPath);

    {
        /* ensure that the parent directory exists */
        android::base::ScopedCPtr<char> parentPath(path_parent(uc->iniPath, 1));
        if (parentPath == NULL) {
            derror("Weird: Can't find parent of user-config file: %s",
                   uc->iniPath);
            return NULL;
        }

        if (!path_exists(parentPath.get())) {
            if (!inAndroidBuild) {
                derror("Weird: No content path for this AVD: %s",
                       parentPath.get());
                return NULL;
            }
            DD("creating missing directory: %s", parentPath.get());
            if (path_mkdir_if_needed(parentPath.get(), 0755) < 0) {
                derror("Using empty user-config, can't create %s: %s",
                       parentPath.get(), strerror(errno));
                return NULL;
            }
        }
    }

    if (path_exists(uc->iniPath)) {
        DD("reading user-config file");
        ini = iniFile_newFromFile(uc->iniPath);
        if (ini == NULL) {
            dwarning("Can't read user-config file: %s\nUsing default values",
                     uc->iniPath);
        }
    }

    if (ini != NULL) {
        uc->windowX = iniFile_getInteger(ini, KEY_WINDOW_X, DEFAULT_X);
        DD("    found %s = %d", KEY_WINDOW_X, uc->windowX);

        uc->windowY = iniFile_getInteger(ini, KEY_WINDOW_Y, DEFAULT_Y);
        DD("    found %s = %d", KEY_WINDOW_Y, uc->windowY);

        if (iniFile_hasKey(ini, KEY_UUID)) {
            uc->uuid = (uint64_t) iniFile_getInt64(ini, KEY_UUID, 0LL);
            needUUID = 0;
            DD("    found %s = %lld", KEY_UUID, uc->uuid);
        }

        iniFile_free(ini);
    }
    else {
        uc->windowX  = DEFAULT_X;
        uc->windowY  = DEFAULT_Y;
        uc->changed  = 1;
    }

    /* Generate a 64-bit UUID if necessary. We simply take the
     * current time, which avoids any privacy-related value.
     */
    if (needUUID) {
        struct timeval  tm;

        gettimeofday( &tm, NULL );
        uc->uuid    = (uint64_t)tm.tv_sec*1000 + tm.tv_usec/1000;
        uc->changed = 1;
        DD("    Generated UUID = %lld", uc->uuid);
    }

    return uc.release();
}


uint64_t
auserConfig_getUUID( AUserConfig*  uconfig )
{
    return uconfig->uuid;
}

void
auserConfig_getWindowPos( AUserConfig*  uconfig, int  *pX, int  *pY )
{
    *pX = uconfig->windowX;
    *pY = uconfig->windowY;
}


void
auserConfig_setWindowPos( AUserConfig*  uconfig, int  x, int  y )
{
    if (x != uconfig->windowX || y != uconfig->windowY) {
        uconfig->windowX = x;
        uconfig->windowY = y;
        uconfig->changed = 1;
    }
}

/* Save the user configuration back to the content directory.
 * Should be used in an atexit() handler */
void
auserConfig_save( AUserConfig*  uconfig )
{
    CIniFile* ini;

    if (uconfig->changed == 0) {
        D("User-config was not changed.");
        return;
    }

    ini = iniFile_newEmpty(uconfig->iniPath);
    if (ini == NULL) {
        D("Weird: can't allocate user-config iniFile?");
        return;
    }

    iniFile_setInteger(ini, KEY_WINDOW_X, uconfig->windowX);
    iniFile_setInteger(ini, KEY_WINDOW_Y, uconfig->windowY);
    iniFile_setInt64(ini, KEY_UUID, uconfig->uuid);
    if (iniFile_saveToFile(ini, uconfig->iniPath) < 0) {
        dwarning("could not save user configuration: %s: %s",
                 uconfig->iniPath, strerror(errno));
    } else {
        D("User configuration saved to %s", uconfig->iniPath);
    }
    iniFile_free(ini);
}
