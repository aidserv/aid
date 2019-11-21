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
#include "android/avd/info.h"

#include "android/android.h"
#include "android/avd/util.h"
#include "android/avd/keys.h"
#include "android/base/ArraySize.h"
#include "android/emulation/bufprint_config_dirs.h"
#include "android/utils/bufprint.h"
#include "android/utils/debug.h"
#include "android/utils/dirscanner.h"
#include "android/utils/file_data.h"
#include "android/utils/filelock.h"
#include "android/utils/path.h"
#include "android/utils/property_file.h"
#include "android/utils/string.h"
#include "android/utils/tempfile.h"

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

/* global variables - see android/globals.h */
AvdInfoParams   android_avdParams[1];
AvdInfo*        android_avdInfo;

/* for debugging */
#define  D(...)   VERBOSE_PRINT(init,__VA_ARGS__)
#define  DD(...)  VERBOSE_PRINT(avd_config,__VA_ARGS__)

/* technical note on how all of this is supposed to work:
 *
 * Each AVD corresponds to a "content directory" that is used to
 * store persistent disk images and configuration files. Most remarkable
 * are:
 *
 * - a "config.ini" file used to hold configuration information for the
 *   AVD
 *
 * - mandatory user data image ("userdata-qemu.img") and cache image
 *   ("cache.img")
 *
 * - optional mutable system image ("system-qemu.img"), kernel image
 *   ("kernel-qemu") and read-only ramdisk ("ramdisk.img")
 *
 * When starting up an AVD, the emulator looks for relevant disk images
 * in the content directory. If it doesn't find a given image there, it
 * will try to search in the list of system directories listed in the
 * 'config.ini' file through one of the following (key,value) pairs:
 *
 *    images.sysdir.1 = <first search path>
 *    images.sysdir.2 = <second search path>
 *
 * The search paths can be absolute, or relative to the root SDK installation
 * path (which is determined from the emulator program's location, or from the
 * ANDROID_SDK_ROOT environment variable).
 *
 * Individual image disk search patch can be over-riden on the command-line
 * with one of the usual options.
 */

/* certain disk image files are mounted read/write by the emulator
 * to ensure that several emulators referencing the same files
 * do not corrupt these files, we need to lock them and respond
 * to collision depending on the image type.
 *
 * the enumeration below is used to record information about
 * each image file path.
 *
 * READONLY means that the file will be mounted read-only
 * and this doesn't need to be locked. must be first in list
 *
 * MUSTLOCK means that the file should be locked before
 * being mounted by the emulator
 *
 * TEMPORARY means that the file has been copied to a
 * temporary image, which can be mounted read/write
 * but doesn't require locking.
 */
typedef enum {
    IMAGE_STATE_READONLY,     /* unlocked */
    IMAGE_STATE_MUSTLOCK,     /* must be locked */
    IMAGE_STATE_LOCKED,       /* locked */
    IMAGE_STATE_LOCKED_EMPTY, /* locked and empty */
    IMAGE_STATE_TEMPORARY,    /* copied to temp file (no lock needed) */
} AvdImageState;

struct AvdInfo {
    /* for the Android build system case */
    char      inAndroidBuild;
    char      isEncryptionEnabled;
    char*     androidOut;
    char*     androidBuildRoot;
    char*     targetArch;
    char*     targetAbi;

    /* for the normal virtual device case */
    char*     deviceName;
    char*     sdkRootPath;
    char*     searchPaths[ MAX_SEARCH_PATHS ];
    int       numSearchPaths;
    char*     contentPath;
    char*     rootIniPath;
    CIniFile* rootIni;   /* root <foo>.ini file, empty if missing */
    CIniFile* configIni; /* virtual device's config.ini, NULL if missing */
    CIniFile* skinHardwareIni; /* skin-specific hardware.ini */

    /* for both */
    int       apiLevel;
    int       incrementalVersion;

    /* For preview releases where we don't know the exact API level this flag
     * indicates that at least we know it's M+ (for some code that needs to
     * select either legacy or modern operation mode.
     */
    bool      isMarshmallowOrHigher;
    bool      isGoogleApis;
    bool      isUserBuild;
    AvdFlavor flavor;
    char*     skinName;     /* skin name */
    char*     skinDirPath;  /* skin directory */
    char*     coreHardwareIniPath;  /* core hardware.ini path */

    FileData  buildProperties[1];  /* build.prop file */
    FileData  bootProperties[1];   /* boot.prop file */

    /* image files */
    char*     imagePath [ AVD_IMAGE_MAX ];
    char      imageState[ AVD_IMAGE_MAX ];
};


void
avdInfo_free( AvdInfo*  i )
{
    if (i) {
        int  nn;

        for (nn = 0; nn < AVD_IMAGE_MAX; nn++)
            AFREE(i->imagePath[nn]);

        AFREE(i->skinName);
        AFREE(i->skinDirPath);
        AFREE(i->coreHardwareIniPath);

        fileData_done(i->buildProperties);
        fileData_done(i->bootProperties);

        for (nn = 0; nn < i->numSearchPaths; nn++)
            AFREE(i->searchPaths[nn]);

        i->numSearchPaths = 0;

        if (i->configIni) {
            iniFile_free(i->configIni);
            i->configIni = NULL;
        }

        if (i->skinHardwareIni) {
            iniFile_free(i->skinHardwareIni);
            i->skinHardwareIni = NULL;
        }

        if (i->rootIni) {
            iniFile_free(i->rootIni);
            i->rootIni = NULL;
        }

        AFREE(i->contentPath);
        AFREE(i->sdkRootPath);
        AFREE(i->rootIniPath);
        AFREE(i->targetArch);
        AFREE(i->targetAbi);

        if (i->inAndroidBuild) {
            AFREE(i->androidOut);
            AFREE(i->androidBuildRoot);
        }

        AFREE(i->deviceName);
        AFREE(i);
    }
}

/* list of default file names for each supported image file type */
static const char*  const  _imageFileNames[ AVD_IMAGE_MAX ] = {
#define  _AVD_IMG(x,y,z)  y,
    AVD_IMAGE_LIST
#undef _AVD_IMG
};

/***************************************************************
 ***************************************************************
 *****
 *****    UTILITY FUNCTIONS
 *****
 *****  The following functions do not depend on the AvdInfo
 *****  structure and could easily be moved elsewhere.
 *****
 *****/

/* Parse a given config.ini file and extract the list of SDK search paths
 * from it. Returns the number of valid paths stored in 'searchPaths', or -1
 * in case of problem.
 *
 * Relative search paths in the config.ini will be stored as full pathnames
 * relative to 'sdkRootPath'.
 *
 * 'searchPaths' must be an array of char* pointers of at most 'maxSearchPaths'
 * entries.
 */
static int _getSearchPaths(CIniFile* configIni,
                           const char* sdkRootPath,
                           int maxSearchPaths,
                           char** searchPaths) {
    char  temp[PATH_MAX], *p = temp, *end= p+sizeof temp;
    int   nn, count = 0;

    for (nn = 0; nn < maxSearchPaths; nn++) {
        char*  path;

        p = bufprint(temp, end, "%s%d", SEARCH_PREFIX, nn+1 );
        if (p >= end)
            continue;

        path = iniFile_getString(configIni, temp, NULL);
        if (path != NULL) {
            DD("    found image search path: %s", path);
            if (!path_is_absolute(path)) {
                p = bufprint(temp, end, "%s/%s", sdkRootPath, path);
                AFREE(path);
                path = ASTRDUP(temp);
            }
            searchPaths[count++] = path;
        }
    }
    return count;
}

/* Check that an AVD name is valid. Returns 1 on success, 0 otherwise.
 */
static int
_checkAvdName( const char*  name )
{
    int  len  = strlen(name);
    int  len2 = strspn(name, "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                             "abcdefghijklmnopqrstuvwxyz"
                             "0123456789_.-");
    return (len == len2);
}

/* Returns the full path of a given file.
 *
 * If 'fileName' is an absolute path, this returns a simple copy.
 * Otherwise, this returns a new string corresponding to <rootPath>/<fileName>
 *
 * This returns NULL if the paths are too long.
 */
static char*
_getFullFilePath( const char* rootPath, const char* fileName )
{
    if (path_is_absolute(fileName)) {
        return ASTRDUP(fileName);
    } else {
        char temp[PATH_MAX], *p=temp, *end=p+sizeof(temp);

        p = bufprint(temp, end, "%s/%s", rootPath, fileName);
        if (p >= end) {
            return NULL;
        }
        return ASTRDUP(temp);
    }
}

/* check that a given directory contains a valid skin.
 * returns 1 on success, 0 on failure.
 */
static int
_checkSkinPath( const char*  skinPath )
{
    char  temp[MAX_PATH], *p=temp, *end=p+sizeof(temp);

    /* for now, if it has a 'layout' file, it is a valid skin path */
    p = bufprint(temp, end, "%s/layout", skinPath);
    if (p >= end || !path_exists(temp))
        return 0;

    return 1;
}

/* Check that there is a skin named 'skinName' listed from 'skinDirRoot'
 * this returns the full path of the skin directory (after alias expansions),
 * including the skin name, or NULL on failure.
 */
static char*
_checkSkinSkinsDir( const char*  skinDirRoot,
                    const char*  skinName )
{
    DirScanner*  scanner;
    char*        result;
    char         temp[MAX_PATH], *p = temp, *end = p + sizeof(temp);

    p = bufprint(temp, end, "%s/skins/%s", skinDirRoot, skinName);
    DD("Probing skin directory: %s", temp);
    if (p >= end || !path_exists(temp)) {
        DD("    ignore bad skin directory %s", temp);
        return NULL;
    }

    /* first, is this a normal skin directory ? */
    if (_checkSkinPath(temp)) {
        /* yes */
        DD("    found skin directory: %s", temp);
        return ASTRDUP(temp);
    }

    /* second, is it an alias to another skin ? */
    *p      = 0;
    result  = NULL;
    scanner = dirScanner_new(temp);
    if (scanner != NULL) {
        for (;;) {
            const char*  file = dirScanner_next(scanner);

            if (file == NULL)
                break;

            if (strncmp(file, "alias-", 6) || file[6] == 0)
                continue;

            p = bufprint(temp, end, "%s/skins/%s", skinDirRoot, file+6);
            if (p < end && _checkSkinPath(temp)) {
                /* yes, it's an alias */
                DD("    skin alias '%s' points to skin directory: %s",
                   file+6, temp);
                result = ASTRDUP(temp);
                break;
            }
        }
        dirScanner_free(scanner);
    }
    return result;
}

/* try to see if the skin name leads to a magic skin or skin path directly
 * returns 1 on success, 0 on error.
 *
 * on success, this sets up '*pSkinName' and '*pSkinDir'
 */
static int
_getSkinPathFromName( const char*  skinName,
                      const char*  sdkRootPath,
                      char**       pSkinName,
                      char**       pSkinDir )
{
    char  temp[PATH_MAX], *p=temp, *end=p+sizeof(temp);

    /* if the skin name has the format 'NNNNxNNN' where
    * NNN is a decimal value, then this is a 'magic' skin
    * name that doesn't require a skin directory
    */
    if (isdigit(skinName[0])) {
        int  width, height;
        if (sscanf(skinName, "%dx%d", &width, &height) == 2) {
            D("'magic' skin format detected: %s", skinName);
            *pSkinName = ASTRDUP(skinName);
            *pSkinDir  = NULL;
            return 1;
        }
    }

    /* is the skin name a direct path to the skin directory ? */
    if (path_is_absolute(skinName) && _checkSkinPath(skinName)) {
        goto FOUND_IT;
    }

    /* is the skin name a relative path from the SDK root ? */
    p = bufprint(temp, end, "%s/%s", sdkRootPath, skinName);
    if (p < end && _checkSkinPath(temp)) {
        skinName = temp;
        goto FOUND_IT;
    }

    /* nope */
    return 0;

FOUND_IT:
    if (path_split(skinName, pSkinDir, pSkinName) < 0) {
        derror("malformed skin name: %s", skinName);
        return 0;
    }
    D("found skin '%s' in directory: %s", *pSkinName, *pSkinDir);
    return 1;
}

/***************************************************************
 ***************************************************************
 *****
 *****    NORMAL VIRTUAL DEVICE SUPPORT
 *****
 *****/

/* compute path to the root SDK directory
 * assume we are in $SDKROOT/tools/emulator[.exe]
 */
static int
_avdInfo_getSdkRoot( AvdInfo*  i )
{

    i->sdkRootPath = path_getSdkRoot();
    if (i->sdkRootPath == NULL) {
        derror("can't find SDK installation directory");
        return -1;
    }
    return 0;
}

/* parse the root config .ini file. it is located in
 * ~/.android/avd/<name>.ini or Windows equivalent
 */
static int
_avdInfo_getRootIni( AvdInfo*  i )
{
    i->rootIniPath = path_getRootIniPath( i->deviceName );

    if (i->rootIniPath == NULL) {
        derror("unknown virtual device name: '%s'", i->deviceName);
        return -1;
    }

    D("Android virtual device file at: %s", i->rootIniPath);

    i->rootIni = iniFile_newFromFile(i->rootIniPath);

    if (i->rootIni == NULL) {
        derror("Corrupt virtual device config file!");
        return -1;
    }
    return 0;
}

/* Returns the AVD's content path, i.e. the directory that contains
 * the AVD's content files (e.g. data partition, cache, sd card, etc...).
 *
 * We extract this by parsing the root config .ini file, looking for
 * a "path" elements.
 */
static int
_avdInfo_getContentPath( AvdInfo*  i )
{
    char temp[PATH_MAX], *p=temp, *end=p+sizeof(temp);

    i->contentPath = iniFile_getString(i->rootIni, ROOT_ABS_PATH_KEY, NULL);

    if (i->contentPath == NULL) {
        derror("bad config: %s",
               "virtual device file lacks a "ROOT_ABS_PATH_KEY" entry");
        return -1;
    }

    if (!path_is_dir(i->contentPath)) {
        // If the absolute path doesn't match an actual directory, try
        // the relative path if present.
        const char* relPath = iniFile_getString(i->rootIni, ROOT_REL_PATH_KEY, NULL);
        if (relPath != NULL) {
            p = bufprint_config_path(temp, end);
            p = bufprint(p, end, PATH_SEP "%s", relPath);
            if (p < end && path_is_dir(temp)) {
                str_reset(&i->contentPath, temp);
            }
        }
    }

    D("virtual device content at %s", i->contentPath);
    return 0;
}

static int
_avdInfo_getApiLevel(AvdInfo* i, bool* isMarshmallowOrHigher)
{
    char*       target;
    const char* p;
    const int   defaultLevel = 1000;
    int         level        = defaultLevel;

#    define ROOT_TARGET_KEY   "target"

    target = iniFile_getString(i->rootIni, ROOT_TARGET_KEY, NULL);
    if (target == NULL) {
        D("No target field in root AVD .ini file?");
        D("Defaulting to API level %d", level);
        return level;
    }

    DD("Found target field in root AVD .ini file: '%s'", target);

    /* There are two acceptable formats for the target key.
     *
     * 1/  android-<level>
     * 2/  <vendor-name>:<add-on-name>:<level>
     *
     * Where <level> can be either a _name_ (for experimental/preview SDK builds)
     * or a decimal number. Note that if a _name_, it can start with a digit.
     */

    /* First, extract the level */
    if (!memcmp(target, "android-", 8))
        p = target + 8;
    else {
        /* skip two columns */
        p = strchr(target, ':');
        if (p != NULL) {
            p = strchr(p+1, ':');
            if (p != NULL)
                p += 1;
        }
    }
    if (p == NULL || !isdigit(*p)) {
        // preview versions usually have a single letter instead of the API
        // level.
        if (p && isalpha(p[0]) && p[1] == 0) {
            level = avdInfo_getApiLevelFromLetter(p[0]);
            if (level > 99 && toupper(p[0]) >= 'M') {
                *isMarshmallowOrHigher = true;
            }
        } else {
            goto NOT_A_NUMBER;
        }
    } else {
        char* end;
        long  val = strtol(p, &end, 10);
        if (end == NULL || *end != '\0' || val != (int)val) {
            goto NOT_A_NUMBER;
        }
        level = (int)val;

        /* Sanity check, we don't support anything prior to Android 1.5 */
        if (level < 3)
            level = 3;

        D("Found AVD target API level: %d", level);
    }
EXIT:
    AFREE(target);
    return level;

NOT_A_NUMBER:
    if (p == NULL) {
        D("Invalid target field in root AVD .ini file");
    } else {
        D("Target AVD api level is not a number");
    }
    D("Defaulting to API level %d", level);
    goto EXIT;
}

bool
avdInfo_isGoogleApis(const AvdInfo* i) {
    return i->isGoogleApis;
}

bool
avdInfo_isUserBuild(const AvdInfo* i) {
    return i->isUserBuild;
}

AvdFlavor avdInfo_getAvdFlavor(const AvdInfo* i) {
    return i->flavor;
}

int
avdInfo_getApiLevel(const AvdInfo* i) {
    return i->apiLevel;
}

// This information was taken from the SDK Manager:
// Appearances & Behavior > System Settings > Android SDK > SDK Platforms
static const struct {
    int apiLevel;
    const char* dessertName;
    const char* fullName;
} kApiLevelInfo[] = {
    { 10, "Gingerbread", "2.3.3 (Gingerbread) - API 10 (Rev 2)" },
    { 14, "Ice Cream Sandwich", "4.0 (Ice Cream Sandwich) - API 14 (Rev 4)" },
    { 15, "Ice Cream Sandwich", "4.0.3 (Ice Cream Sandwich) - API 15 (Rev 5)" },
    { 16, "Jelly Bean", "4.1 (Jelly Bean) - API 16 (Rev 5)" },
    { 17, "Jelly Bean", "4.2 (Jelly Bean) - API 17 (Rev 3)" },
    { 18, "Jelly Bean", "4.3 (Jelly Bean) - API 18 (Rev 3)" },
    { 19, "KitKat", "4.4 (KitKat) - API 19 (Rev 4)" },
    { 20, "KitKat", "4.4 (KitKat Wear) - API 20 (Rev 2)" },
    { 21, "Lollipop", "5.0 (Lollipop) - API 21 (Rev 2)" },
    { 22, "Lollipop", "5.1 (Lollipop) - API 22 (Rev 2)" },
    { 23, "Marshmallow", "6.0 (Marshmallow) - API 23 (Rev 1)" },
    { 24, "Nougat", "7.0 (Nougat) - API 24" },
    { 25, "Nougat", "7.1 (Nougat) - API 25" },
    { 26, "O Preview", "API 26" },
};

const char* avdInfo_getApiDessertName(int apiLevel) {
    int i = 0;
    for (; i < ARRAY_SIZE(kApiLevelInfo); ++i) {
        if (kApiLevelInfo[i].apiLevel == apiLevel) {
            return kApiLevelInfo[i].dessertName;
        }
    }
    return "";
}

void avdInfo_getFullApiName(int apiLevel, char* nameStr, int strLen) {
    if (apiLevel < 0 || apiLevel > 99) {
        strncpy(nameStr, "Unknown API version", strLen);
        return;
    }

    int i = 0;
    for (; i < ARRAY_SIZE(kApiLevelInfo); ++i) {
        if (kApiLevelInfo[i].apiLevel == apiLevel) {
            strncpy(nameStr, kApiLevelInfo[i].fullName, strLen);
            return;
        }
    }
    snprintf(nameStr, strLen, "API %d", apiLevel);
}

int avdInfo_getApiLevelFromLetter(char letter) {
    const char letterUpper = toupper(letter);
    int i = 0;
    for (; i < ARRAY_SIZE(kApiLevelInfo); ++i) {
        if (toupper(kApiLevelInfo[i].dessertName[0]) == letterUpper) {
            return kApiLevelInfo[i].apiLevel;
        }
    }
    return 1000;
}

/* Look for a named file inside the AVD's content directory.
 * Returns NULL if it doesn't exist, or a strdup() copy otherwise.
 */
static char*
_avdInfo_getContentFilePath(const AvdInfo*  i, const char* fileName)
{
    char temp[MAX_PATH], *p = temp, *end = p + sizeof(temp);

    p = bufprint(p, end, "%s/%s", i->contentPath, fileName);
    if (p >= end) {
        derror("can't access virtual device content directory");
        return NULL;
    }
    if (!path_exists(temp)) {
        return NULL;
    }
    return ASTRDUP(temp);
}

/* find and parse the config.ini file from the content directory */
static int
_avdInfo_getConfigIni(AvdInfo*  i)
{
    char*  iniPath = _avdInfo_getContentFilePath(i, CORE_CONFIG_INI);

    /* Allow non-existing config.ini */
    if (iniPath == NULL) {
        D("virtual device has no config file - no problem");
        return 0;
    }

    D("virtual device config file: %s", iniPath);
    i->configIni = iniFile_newFromFile(iniPath);
    AFREE(iniPath);

    if (i->configIni == NULL) {
        derror("bad config: %s",
               "virtual device has corrupted " CORE_CONFIG_INI);
        return -1;
    }
    return 0;
}

/* The AVD's config.ini contains a list of search paths (all beginning
 * with SEARCH_PREFIX) which are directory locations searched for
 * AVD platform files.
 */
static bool
_avdInfo_getSearchPaths( AvdInfo*  i )
{
    if (i->configIni == NULL)
        return true;

    i->numSearchPaths = _getSearchPaths( i->configIni,
                                         i->sdkRootPath,
                                         MAX_SEARCH_PATHS,
                                         i->searchPaths );
    if (i->numSearchPaths == 0) {
        derror("no search paths found in this AVD's configuration.\n"
               "Weird, the AVD's " CORE_CONFIG_INI " file is malformed. "
               "Try re-creating it.\n");
        return false;
    }
    else
        DD("found a total of %d search paths for this AVD", i->numSearchPaths);
    return true;
}

/* Search a file in the SDK search directories. Return NULL if not found,
 * or a strdup() otherwise.
 */
static char*
_avdInfo_getSdkFilePath(const AvdInfo*  i, const char*  fileName)
{
    char temp[MAX_PATH], *p = temp, *end = p + sizeof(temp);

    do {
        /* try the search paths */
        int  nn;

        for (nn = 0; nn < i->numSearchPaths; nn++) {
            const char* searchDir = i->searchPaths[nn];

            p = bufprint(temp, end, "%s/%s", searchDir, fileName);
            if (p < end && path_exists(temp)) {
                DD("found %s in search dir: %s", fileName, searchDir);
                goto FOUND;
            }
            DD("    no %s in search dir: %s", fileName, searchDir);
        }

        return NULL;

    } while (0);

FOUND:
    return ASTRDUP(temp);
}

/* Search for a file in the content directory, and if not found, in the
 * SDK search directory. Returns NULL if not found.
 */
static char*
_avdInfo_getContentOrSdkFilePath(const AvdInfo*  i, const char*  fileName)
{
    char*  path;

    path = _avdInfo_getContentFilePath(i, fileName);
    if (path)
        return path;

    path = _avdInfo_getSdkFilePath(i, fileName);
    if (path)
        return path;

    return NULL;
}

#if 0
static int
_avdInfo_findContentOrSdkImage(const AvdInfo* i, AvdImageType id)
{
    const char* fileName = _imageFileNames[id];
    char*       path     = _avdInfo_getContentOrSdkFilePath(i, fileName);

    i->imagePath[id]  = path;
    i->imageState[id] = IMAGE_STATE_READONLY;

    if (path == NULL)
        return -1;
    else
        return 0;
}
#endif

/* Returns path to the core hardware .ini file. This contains the
 * hardware configuration that is read by the core. The content of this
 * file is auto-generated before launching a core, but we need to know
 * its path before that.
 */
static int
_avdInfo_getCoreHwIniPath( AvdInfo* i, const char* basePath )
{
    i->coreHardwareIniPath = _getFullFilePath(basePath, CORE_HARDWARE_INI);
    if (i->coreHardwareIniPath == NULL) {
        DD("Path too long for %s: %s", CORE_HARDWARE_INI, basePath);
        return -1;
    }
    D("using core hw config path: %s", i->coreHardwareIniPath);
    return 0;
}


static void
_avdInfo_readPropertyFile(const AvdInfo* i,
                          const char* filePath,
                          FileData* data) {
    int ret = fileData_initFromFile(data, filePath);
    if (ret < 0) {
        D("Error reading property file %s: %s", filePath, strerror(-ret));
    } else {
        D("Read property file at %s", filePath);
    }
}


static void
_avdInfo_extractBuildProperties(AvdInfo* i) {
    i->targetArch = propertyFile_getTargetArch(i->buildProperties);
    if (!i->targetArch) {
        str_reset(&i->targetArch, "arm");
        D("Cannot find target CPU architecture, defaulting to '%s'",
          i->targetArch);
    }
    i->targetAbi = propertyFile_getTargetAbi(i->buildProperties);
    if (!i->targetAbi) {
        str_reset(&i->targetAbi, "armeabi");
        D("Cannot find target CPU ABI, defaulting to '%s'",
          i->targetAbi);
    }
    if (!i->apiLevel) {
        // Note: for regular AVDs, the API level is already extracted
        // from config.ini, besides, for older SDK platform images,
        // there is no build.prop file and the following function
        // would always return 1000, making the AVD unbootable!.
        i->apiLevel = propertyFile_getApiLevel(i->buildProperties);
        if (i->apiLevel < 3) {
            i->apiLevel = 3;
            D("Cannot find target API level, defaulting to %d",
            i->apiLevel);
        }
    }

    i->flavor = propertyFile_getAvdFlavor(i->buildProperties);

    i->isGoogleApis = propertyFile_isGoogleApis(i->buildProperties);
    i->isUserBuild = propertyFile_isUserBuild(i->buildProperties);
    i->incrementalVersion = propertyFile_getInt(
        i->buildProperties,
        "ro.build.version.incremental",
        -1,
        NULL);
}


static void
_avdInfo_getPropertyFile(AvdInfo* i,
                         const char* propFileName,
                         FileData* data ) {
    char* filePath = _avdInfo_getContentOrSdkFilePath(i, propFileName);
    if (!filePath) {
        D("No %s property file found.", propFileName);
        return;
    }

    _avdInfo_readPropertyFile(i, filePath, data);
    free(filePath);
}

AvdInfo*
avdInfo_new( const char*  name, AvdInfoParams*  params )
{
    AvdInfo*  i;

    if (name == NULL)
        return NULL;

    if (!_checkAvdName(name)) {
        derror("virtual device name contains invalid characters");
        return NULL;
    }

    ANEW0(i);
    str_reset(&i->deviceName, name);

    if ( _avdInfo_getSdkRoot(i) < 0     ||
         _avdInfo_getRootIni(i) < 0     ||
         _avdInfo_getContentPath(i) < 0 ||
         _avdInfo_getConfigIni(i)   < 0 ||
         _avdInfo_getCoreHwIniPath(i, i->contentPath) < 0 )
        goto FAIL;

    i->apiLevel = _avdInfo_getApiLevel(i, &i->isMarshmallowOrHigher);

    /* look for image search paths. handle post 1.1/pre cupcake
     * obsolete SDKs.
     */
    if (!_avdInfo_getSearchPaths(i)) {
        goto FAIL;
    }

    // Find the build.prop and boot.prop files and read them.
    _avdInfo_getPropertyFile(i, "build.prop", i->buildProperties);
    _avdInfo_getPropertyFile(i, "boot.prop", i->bootProperties);

    _avdInfo_extractBuildProperties(i);

    /* don't need this anymore */
    iniFile_free(i->rootIni);
    i->rootIni = NULL;

    return i;

FAIL:
    avdInfo_free(i);
    return NULL;
}

/***************************************************************
 ***************************************************************
 *****
 *****    ANDROID BUILD SUPPORT
 *****
 *****    The code below corresponds to the case where we're
 *****    starting the emulator inside the Android build
 *****    system. The main differences are that:
 *****
 *****    - the $ANDROID_PRODUCT_OUT directory is used as the
 *****      content file.
 *****
 *****    - built images must not be modified by the emulator,
 *****      so system.img must be copied to a temporary file
 *****      and userdata.img must be copied to userdata-qemu.img
 *****      if the latter doesn't exist.
 *****
 *****    - the kernel and default skin directory are taken from
 *****      prebuilt
 *****
 *****    - there is no root .ini file, or any config.ini in
 *****      the content directory, no SDK images search path.
 *****/

/* Read a hardware.ini if it is located in the skin directory */
static int
_avdInfo_getBuildSkinHardwareIni( AvdInfo*  i )
{
    char* skinName;
    char* skinDirPath;

    avdInfo_getSkinInfo(i, &skinName, &skinDirPath);
    if (skinDirPath == NULL)
        return 0;

    int result = avdInfo_getSkinHardwareIni(i, skinName, skinDirPath);

    AFREE(skinName);
    AFREE(skinDirPath);

    return result;
}

int avdInfo_getSkinHardwareIni( AvdInfo* i, char* skinName, char* skinDirPath)
{
    char  temp[PATH_MAX], *p=temp, *end=p+sizeof(temp);

    p = bufprint(temp, end, "%s/%s/hardware.ini", skinDirPath, skinName);
    if (p >= end || !path_exists(temp)) {
        DD("no skin-specific hardware.ini in %s", skinDirPath);
        return 0;
    }

    D("found skin-specific hardware.ini: %s", temp);
    if (i->skinHardwareIni != NULL)
        iniFile_free(i->skinHardwareIni);
    i->skinHardwareIni = iniFile_newFromFile(temp);
    if (i->skinHardwareIni == NULL)
        return -1;

    return 0;
}

int avdInfo_isEncryptionEnabledInBuild(const AvdInfo* i) {
    return (i->inAndroidBuild && i->isEncryptionEnabled);
}

AvdInfo*
avdInfo_newForAndroidBuild( const char*     androidBuildRoot,
                            const char*     androidOut,
                            AvdInfoParams*  params )
{
    AvdInfo*  i;

    ANEW0(i);

    i->inAndroidBuild   = 1;
    str_reset(&i->androidBuildRoot, androidBuildRoot);
    str_reset(&i->androidOut, androidOut);
    str_reset(&i->contentPath, androidOut);

    // Find the build.prop file and read it.
    char* buildPropPath = path_getBuildBuildProp(i->androidOut);
    if (buildPropPath) {
        _avdInfo_readPropertyFile(i, buildPropPath, i->buildProperties);
        free(buildPropPath);
    }

    // FInd the boot.prop file and read it.
    char* bootPropPath = path_getBuildBootProp(i->androidOut);
    if (bootPropPath) {
        _avdInfo_readPropertyFile(i, bootPropPath, i->bootProperties);
        free(bootPropPath);
    }

    _avdInfo_extractBuildProperties(i);

    if (i->apiLevel >= 25) {
         /*
           copy encryptionkey.img
        */
        char encryptionimg_path[1024];
        snprintf(encryptionimg_path, sizeof(encryptionimg_path),
                "%s/device/generic/goldfish/data/etc/encryptionkey.img",
                androidBuildRoot);
        const bool encryption_enabled = path_exists(encryptionimg_path);
        if (!encryption_enabled) {
            dwarning("%s does not exists\n", encryptionimg_path);
            i->isEncryptionEnabled = 0;
        } else {
            i->isEncryptionEnabled = 1;
            char dest_encryptionimg_path[1024];
            snprintf(dest_encryptionimg_path, sizeof(dest_encryptionimg_path),
                    "%s/encryptionkey.img",
                    androidOut);
            if (!path_exists(dest_encryptionimg_path)) {
                if(path_copy_file(dest_encryptionimg_path, encryptionimg_path) != 0) {
                    derror("cannot copy from %s to %s\n", encryptionimg_path,
                            dest_encryptionimg_path);
                    goto FAIL;
                }
            }
        }
    }
    str_reset(&i->deviceName, "<build>");

    /* out/target/product/<name>/config.ini, if exists, provide configuration
     * from build files. */
    if (_avdInfo_getConfigIni(i) < 0 ||
        _avdInfo_getCoreHwIniPath(i, i->androidOut) < 0)
        goto FAIL;

    /* Read the build skin's hardware.ini, if any */
    _avdInfo_getBuildSkinHardwareIni(i);

    return i;

FAIL:
    avdInfo_free(i);
    return NULL;
}

const char*
avdInfo_getName( const AvdInfo*  i )
{
    return i ? i->deviceName : NULL;
}

const char*
avdInfo_getImageFile( const AvdInfo*  i, AvdImageType  imageType )
{
    if (i == NULL || (unsigned)imageType >= AVD_IMAGE_MAX)
        return NULL;

    return i->imagePath[imageType];
}

uint64_t
avdInfo_getImageFileSize( const AvdInfo*  i, AvdImageType  imageType )
{
    const char* file = avdInfo_getImageFile(i, imageType);
    uint64_t    size;

    if (file == NULL)
        return 0ULL;

    if (path_get_size(file, &size) < 0)
        return 0ULL;

    return size;
}

int
avdInfo_isImageReadOnly( const AvdInfo*  i, AvdImageType  imageType )
{
    if (i == NULL || (unsigned)imageType >= AVD_IMAGE_MAX)
        return 1;

    return (i->imageState[imageType] == IMAGE_STATE_READONLY);
}

char*
avdInfo_getKernelPath( const AvdInfo*  i )
{
    const char* imageName = _imageFileNames[ AVD_IMAGE_KERNEL ];

    char*  kernelPath = _avdInfo_getContentOrSdkFilePath(i, imageName);

    do {
        if (kernelPath || !i->inAndroidBuild)
            break;

        /* When in the Android build, look into the prebuilt directory
         * for our target architecture.
         */
        char temp[PATH_MAX], *p = temp, *end = p + sizeof(temp);
        const char* suffix = "";

        // If the target ABI is armeabi-v7a, then look for
        // kernel-qemu-armv7 instead of kernel-qemu in the prebuilt
        // directory.
        if (!strcmp(i->targetAbi, "armeabi-v7a")) {
            suffix = "-armv7";
        }

        p = bufprint(temp, end, "%s/kernel", i->androidOut);
        if (p < end && path_exists(temp)) {
            str_reset(&kernelPath, temp);
            break;
        }

        p = bufprint(temp, end, "%s/prebuilts/qemu-kernel/%s/kernel-qemu%s",
                     i->androidBuildRoot, i->targetArch, suffix);
        if (p >= end || !path_exists(temp)) {
            derror("bad workspace: cannot find prebuilt kernel in: %s", temp);
            kernelPath = NULL;
            break;
        }
        str_reset(&kernelPath, temp);

    } while (0);

    return kernelPath;
}

char*
avdInfo_getRanchuKernelPath( const AvdInfo*  i )
{
    const char* imageName = _imageFileNames[ AVD_IMAGE_KERNELRANCHU ];

    char*  kernelPath = _avdInfo_getContentOrSdkFilePath(i, imageName);

    do {
        if (kernelPath || !i->inAndroidBuild)
            break;

        /* When in the Android build, look into the prebuilt directory
         * for our target architecture.
         */
        char temp[PATH_MAX], *p = temp, *end = p + sizeof(temp);
        const char* suffix = "";

        if (!strcmp(i->targetAbi, "mips32r6")) {
            /* mips/ranchu holds distinct images for mips & mips32r6 */
            suffix = "-mips32r6";
        }

        p = bufprint(temp, end, "%s/prebuilts/qemu-kernel/%s/ranchu/kernel-qemu%s",
                     i->androidBuildRoot, i->targetArch, suffix);
        if (p >= end || !path_exists(temp)) {
            /* arm64 and mips64 are special: their kernel-qemu is actually kernel-ranchu */
            if (!strcmp(i->targetArch, "arm64") || !strcmp(i->targetArch, "mips64")) {
                return avdInfo_getKernelPath(i);
            } else {
                derror("bad workspace: cannot find prebuilt ranchu kernel in: %s", temp);
                kernelPath = NULL;
                break;
            }
        }
        str_reset(&kernelPath, temp);
    } while (0);

    return kernelPath;
}


char*
avdInfo_getRamdiskPath( const AvdInfo* i )
{
    const char* imageName = _imageFileNames[ AVD_IMAGE_RAMDISK ];
    return _avdInfo_getContentOrSdkFilePath(i, imageName);
}

char*  avdInfo_getCachePath( const AvdInfo*  i )
{
    const char* imageName = _imageFileNames[ AVD_IMAGE_CACHE ];
    return _avdInfo_getContentFilePath(i, imageName);
}

char*  avdInfo_getDefaultCachePath( const AvdInfo*  i )
{
    const char* imageName = _imageFileNames[ AVD_IMAGE_CACHE ];
    return _getFullFilePath(i->contentPath, imageName);
}

char*  avdInfo_getSdCardPath( const AvdInfo* i )
{
    const char* imageName = _imageFileNames[ AVD_IMAGE_SDCARD ];
    char*       path;

    /* Special case, the config.ini can have a SDCARD_PATH entry
     * that gives the full path to the SD Card.
     */
    if (i->configIni != NULL) {
        path = iniFile_getString(i->configIni, SDCARD_PATH, NULL);
        if (path != NULL) {
            if (path_exists(path))
                return path;

            dwarning("Ignoring invalid SDCard path: %s", path);
            AFREE(path);
        }
    }

    /* Otherwise, simply look into the content directory */
    return _avdInfo_getContentFilePath(i, imageName);
}

char* avdInfo_getEncryptionKeyImagePath(const AvdInfo* i )
{
    const char* imageName = _imageFileNames[ AVD_IMAGE_ENCRYPTIONKEY ];
    return _avdInfo_getContentFilePath(i, imageName);
}

char*
avdInfo_getSnapStoragePath( const AvdInfo* i )
{
    const char* imageName = _imageFileNames[ AVD_IMAGE_SNAPSHOTS ];
    return _avdInfo_getContentFilePath(i, imageName);
}

char*
avdInfo_getSystemImagePath( const AvdInfo*  i )
{
    const char* imageName = _imageFileNames[ AVD_IMAGE_USERSYSTEM ];
    return _avdInfo_getContentFilePath(i, imageName);
}

char*
avdInfo_getSystemInitImagePath( const AvdInfo*  i )
{
    const char* imageName = _imageFileNames[ AVD_IMAGE_INITSYSTEM ];
    return _avdInfo_getContentOrSdkFilePath(i, imageName);
}

char*
avdInfo_getDataImagePath( const AvdInfo*  i )
{
    const char* imageName = _imageFileNames[ AVD_IMAGE_USERDATA ];
    return _avdInfo_getContentFilePath(i, imageName);
}

char*
avdInfo_getDefaultDataImagePath( const AvdInfo*  i )
{
    const char* imageName = _imageFileNames[ AVD_IMAGE_USERDATA ];
    return _getFullFilePath(i->contentPath, imageName);
}

char* avdInfo_getDefaultSystemFeatureControlPath(const AvdInfo* i) {
    char* retVal = _avdInfo_getSdkFilePath(i, "advancedFeatures.ini");
    return retVal;
}

char* avdInfo_getDataInitImagePath(const AvdInfo* i) {
    const char* imageName = _imageFileNames[ AVD_IMAGE_INITDATA ];
    return _avdInfo_getContentOrSdkFilePath(i, imageName);
}

char* avdInfo_getDataInitDirPath(const AvdInfo* i) {
    const char* imageName = _imageFileNames[ AVD_IMAGE_INITZIP ];
    return _avdInfo_getSdkFilePath(i, imageName);
}

int
avdInfo_initHwConfig(const AvdInfo* i, AndroidHwConfig*  hw, bool isQemu2)
{
    int  ret = 0;

    androidHwConfig_init(hw, i->apiLevel);

    /* First read the skin's hardware.ini, if any */
    if (i->skinHardwareIni != NULL) {
        ret = androidHwConfig_read(hw, i->skinHardwareIni);
    }

    /* The device's config.ini can override the skin's values
     * (which is preferable to the opposite order)
     */
    if (ret == 0 && i->configIni != NULL) {
        ret = androidHwConfig_read(hw, i->configIni);
    }

    /* Auto-disable keyboard emulation on sapphire platform builds */
    if (i->androidOut != NULL) {
        char*  p = strrchr(i->androidOut, '/');
        if (p != NULL && !strcmp(p,"sapphire")) {
            hw->hw_keyboard = 0;
        }
    }

    // for api <= 10 there is no multi-touch support in any of the ranchu
    // or goldfish kernels and GUI won't respond as a result;
    // force it to be "touch"
    //
    // for api <= 21 the goldfish kernel is not updated to
    // support multi-touch yet; so just force touch
    // bug: https://code.google.com/p/android/issues/detail?id=199289
    //
    // System images above 10 support multi-touch if they have a ranchu kernel
    // and we're using QEMU2 as indicated by the isQemu2 flag.
    //
    // TODO: There is currently an issue related to this to track the release of
    // system images with ranchu kernels for API 21 and below at:
    // https://code.google.com/p/android/issues/detail?id=200332
    if (i->apiLevel <= 10 || (!isQemu2 && i->apiLevel <= 21)) {
            str_reset(&hw->hw_screen, "touch");
    }

    return ret;
}

const char*
avdInfo_getContentPath( const AvdInfo*  i )
{
    return i->contentPath;
}

const char*
avdInfo_getRootIniPath( const AvdInfo*  i )
{
    return i->rootIniPath;
}

int
avdInfo_inAndroidBuild( const AvdInfo*  i )
{
    return i->inAndroidBuild;
}

char*
avdInfo_getTargetCpuArch(const AvdInfo* i) {
    return ASTRDUP(i->targetArch);
}

char*
avdInfo_getTargetAbi( const AvdInfo* i )
{
    /* For now, we can't get the ABI from SDK AVDs */
    return ASTRDUP(i->targetAbi);
}

char*
avdInfo_getCodeProfilePath( const AvdInfo*  i, const char*  profileName )
{
    char   tmp[MAX_PATH], *p=tmp, *end=p + sizeof(tmp);

    if (i == NULL || profileName == NULL || profileName[0] == 0)
        return NULL;

    if (i->inAndroidBuild) {
        p = bufprint( p, end, "%s" PATH_SEP "profiles" PATH_SEP "%s",
                      i->androidOut, profileName );
    } else {
        p = bufprint( p, end, "%s" PATH_SEP "profiles" PATH_SEP "%s",
                      i->contentPath, profileName );
    }
    return ASTRDUP(tmp);
}

const char*
avdInfo_getCoreHwIniPath( const AvdInfo* i )
{
    return i->coreHardwareIniPath;
}


void
avdInfo_getSkinInfo( const AvdInfo*  i, char** pSkinName, char** pSkinDir )
{
    char*  skinName = NULL;
    char*  skinPath;
    char   temp[PATH_MAX], *p=temp, *end=p+sizeof(temp);

    *pSkinName = NULL;
    *pSkinDir  = NULL;

    /* First, see if the config.ini contains a SKIN_PATH entry that
     * names the full directory path for the skin.
     */
    if (i->configIni != NULL ) {
        skinPath = iniFile_getString( i->configIni, SKIN_PATH, NULL );
        if (skinPath != NULL) {
            /* If this skin name is magic or a direct directory path
            * we have our result right here.
            */
            if (_getSkinPathFromName(skinPath, i->sdkRootPath,
                                     pSkinName, pSkinDir )) {
                AFREE(skinPath);
                return;
            }
        }

        /* The SKIN_PATH entry was not valid, so look at SKIN_NAME */
        D("Warning: " CORE_CONFIG_INI " contains invalid %s entry: %s",
          SKIN_PATH, skinPath);
        AFREE(skinPath);

        skinName = iniFile_getString( i->configIni, SKIN_NAME, NULL );
    }

    if (skinName == NULL) {
        /* If there is no skin listed in the config.ini, try to see if
         * there is one single 'skin' directory in the content directory.
         */
        p = bufprint(temp, end, "%s/skin", i->contentPath);
        if (p < end && _checkSkinPath(temp)) {
            D("using skin content from %s", temp);
            AFREE(i->skinName);
            *pSkinName = ASTRDUP("skin");
            *pSkinDir  = ASTRDUP(i->contentPath);
            return;
        }

        /* otherwise, use the default name */
        skinName = ASTRDUP(SKIN_DEFAULT);
    }

    /* now try to find the skin directory for that name -
     */
    do {
        /* first try the content directory, i.e. $CONTENT/skins/<name> */
        skinPath = _checkSkinSkinsDir(i->contentPath, skinName);
        if (skinPath != NULL)
            break;

#define  PREBUILT_SKINS_ROOT "development/tools/emulator"

        /* if we are in the Android build, try the prebuilt directory */
        if (i->inAndroidBuild) {
            p = bufprint( temp, end, "%s/%s",
                        i->androidBuildRoot, PREBUILT_SKINS_ROOT );
            if (p < end) {
                skinPath = _checkSkinSkinsDir(temp, skinName);
                if (skinPath != NULL)
                    break;
            }

            /* or in the parent directory of the system dir */
            {
                char* parentDir = path_parent(i->androidOut, 1);
                if (parentDir != NULL) {
                    skinPath = _checkSkinSkinsDir(parentDir, skinName);
                    AFREE(parentDir);
                    if (skinPath != NULL)
                        break;
                }
            }
        }

        /* look in the search paths. For each <dir> in the list,
         * look into <dir>/../skins/<name>/ */
        {
            int  nn;
            for (nn = 0; nn < i->numSearchPaths; nn++) {
                char*  parentDir = path_parent(i->searchPaths[nn], 1);
                if (parentDir == NULL)
                    continue;
                skinPath = _checkSkinSkinsDir(parentDir, skinName);
                AFREE(parentDir);
                if (skinPath != NULL)
                  break;
            }
            if (nn < i->numSearchPaths)
                break;
        }

        /* We didn't find anything ! */
        *pSkinName = skinName;
        return;

    } while (0);

    if (path_split(skinPath, pSkinDir, pSkinName) < 0) {
        derror("weird skin path: %s", skinPath);
        AFREE(skinPath);
        return;
    }
    DD("found skin '%s' in directory: %s", *pSkinName, *pSkinDir);
    AFREE(skinPath);
    return;
}

char*
avdInfo_getCharmapFile( const AvdInfo* i, const char* charmapName )
{
    char        fileNameBuff[PATH_MAX];
    const char* fileName;

    if (charmapName == NULL || charmapName[0] == '\0')
        return NULL;

    if (strstr(charmapName, ".kcm") == NULL) {
        snprintf(fileNameBuff, sizeof fileNameBuff, "%s.kcm", charmapName);
        fileName = fileNameBuff;
    } else {
        fileName = charmapName;
    }

    return _avdInfo_getContentOrSdkFilePath(i, fileName);
}

AdbdCommunicationMode avdInfo_getAdbdCommunicationMode(const AvdInfo* i,
                                                       bool isQemu2)
{
    if (isQemu2) {
        // All qemu2-compatible system images support modern communication mode.
        return ADBD_COMMUNICATION_MODE_PIPE;
    }

    if (i->apiLevel < 16 || (i->apiLevel > 99 && !i->isMarshmallowOrHigher)) {
        // QEMU pipe for ADB communication was added in android-4.1.1_r1 API 16
        D("API < 16 or unknown, forcing ro.adb.qemud==0");
        return ADBD_COMMUNICATION_MODE_LEGACY;
    }

    // Ignore property file since all system images have been updated a long
    // time ago to support the pipe service for API level >= 16.
    return ADBD_COMMUNICATION_MODE_PIPE;
}

int avdInfo_getSnapshotPresent(const AvdInfo* i)
{
    if (i->configIni == NULL) {
        return 0;
    } else {
        return iniFile_getBoolean(i->configIni, SNAPSHOT_PRESENT, "no");
    }
}

const FileData* avdInfo_getBootProperties(const AvdInfo* i) {
    return i->bootProperties;
}

const FileData* avdInfo_getBuildProperties(const AvdInfo* i) {
    return i->buildProperties;
}

CIniFile* avdInfo_getConfigIni(const AvdInfo* i) {
    return i->configIni;
}

int avdInfo_getSysImgIncrementalVersion(const AvdInfo* i) {
    return i->incrementalVersion;
}

const char* avdInfo_getTag(const AvdInfo* i) {
    char temp[PATH_MAX];
    char* tagId = "default";
    char* tagDisplay = "Default";
    if (i->configIni) {
        tagId = iniFile_getString(i->configIni, TAG_ID, "default");
        tagDisplay = iniFile_getString(i->configIni, TAG_DISPLAY, "Default");
    }
    snprintf(temp, PATH_MAX, "%s [%s]", tagId, tagDisplay);
    return ASTRDUP(temp);
}

const char* avdInfo_getSdCardSize(const AvdInfo* i) {
    return (i->configIni) ? iniFile_getString(i->configIni, SDCARD_SIZE, "")
                          : NULL;
}
