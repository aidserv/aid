/* Copyright (C) 2011 The Android Open Source Project
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
#include "android/avd/keys.h"
#include "android/avd/util.h"

#include "android/android.h"
#include "android/base/ArraySize.h"
#include "android/emulation/bufprint_config_dirs.h"
#include "android/utils/bufprint.h"
#include "android/utils/debug.h"
#include "android/utils/ini.h"
#include "android/utils/panic.h"
#include "android/utils/path.h"
#include "android/utils/property_file.h"
#include "android/utils/system.h"

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#define D(...) VERBOSE_PRINT(init,__VA_ARGS__)

/* Return the path to the AVD's root configuration .ini file. it is located in
 * ~/.android/avd/<name>.ini or Windows equivalent
 *
 * This file contains the path to the AVD's content directory, which
 * includes its own config.ini.
 */
char*
path_getRootIniPath( const char*  avdName )
{
    char temp[PATH_MAX], *p=temp, *end=p+sizeof(temp);

    p = bufprint_avd_home_path(temp, end);
    p = bufprint(p, end, PATH_SEP "%s.ini", avdName);
    if (p >= end) {
        return NULL;
    }
    if (!path_exists(temp)) {
        return NULL;
    }
    return ASTRDUP(temp);
}

char*
path_getAvdContentPath(const char* avdName)
{
    char temp[PATH_MAX], *p=temp, *end=p+sizeof(temp);
    CIniFile* ini = NULL;
    char*    iniPath = path_getRootIniPath(avdName);
    char*    avdPath = NULL;

    if (iniPath != NULL) {
        ini = iniFile_newFromFile(iniPath);
        if (ini == NULL) {
            APANIC("Could not parse file: %s\n", iniPath);
        }
        AFREE(iniPath);
    } else {
        static const char kHomeSearchDir[] = "$HOME" PATH_SEP ".android" PATH_SEP "avd";
        static const char kSdkHomeSearchDir[] = "$ANDROID_SDK_HOME" PATH_SEP "avd";
        const char* envName = "HOME";
        const char* searchDir = kHomeSearchDir;
        if (getenv("ANDROID_AVD_HOME")) {
            envName = "ANDROID_AVD_HOME";
            searchDir = "$ANDROID_AVD_HOME";
        } else if (getenv("ANDROID_SDK_HOME")) {
            envName = "ANDROID_SDK_HOME";
            searchDir = kSdkHomeSearchDir;
        }
        APANIC("Unknown AVD name [%s], use -list-avds to see valid list.\n"
               "%s is defined but there is no file %s.ini in %s\n"
               "(Note: Directories are searched in the order $ANDROID_AVD_HOME, "
               "%s and %s)\n",
               avdName, envName, avdName, searchDir, kSdkHomeSearchDir,
               kHomeSearchDir);
    }

    avdPath = iniFile_getString(ini, ROOT_ABS_PATH_KEY, NULL);

    if (!path_is_dir(avdPath)) {
        // If the absolute path doesn't match an actual directory, try
        // the relative path if present.
        const char* relPath = iniFile_getString(ini, ROOT_REL_PATH_KEY, NULL);
        if (relPath != NULL) {
            p = bufprint_config_path(temp, end);
            p = bufprint(p, end, PATH_SEP "%s", relPath);
            if (p < end && path_is_dir(temp)) {
                AFREE(avdPath);
                avdPath = ASTRDUP(temp);
            }
        }
    }

    iniFile_free(ini);

    return avdPath;
}

char*
propertyFile_getTargetAbi(const FileData* data) {
    char* abi = propertyFile_getValue((const char*)data->data,
                                      data->size,
                                      "ro.product.cpu.abi");
    if (abi && !strcmp(abi, "mips")) {
        // mips32r6 images show cpu.abi as just mips,
        // but give correct abi in a dalvik property.
        char* abi2 = propertyFile_getValue((const char*)data->data,
                                           data->size,
                                           "dalvik.vm.isa.mips.variant");
        if (abi2 && !strcmp(abi2, "mips32r6")) {
            AFREE(abi);
            abi = abi2;
        } else {
            AFREE(abi2);
        }
    }
    return abi;
}

char*
propertyFile_getTargetArch(const FileData* data) {
    char* ret = propertyFile_getTargetAbi(data);
    if (ret) {
        // Translate ABI name into architecture name.
        // By default, there are the same with a few exceptions.
        static const struct {
            const char* input;
            const char* output;
        } kData[] = {
            { "armeabi", "arm" },
            { "armeabi-v7a", "arm" },
            { "arm64-v8a", "arm64" },
            { "mips32r6", "mips" },
        };
        size_t n;
        for (n = 0; n < sizeof(kData)/sizeof(kData[0]); ++n) {
            if (!strcmp(ret, kData[n].input)) {
                free(ret);
                ret = ASTRDUP(kData[n].output);
                break;
            }
        }
    }
    return ret;
}


int
propertyFile_getInt(const FileData* data, const char* key, int _default,
                    SearchResult* searchResult) {
    char* prop = propertyFile_getValue((const char*)data->data,
                                       data->size,
                                       key);
    if (!prop) {
        if (searchResult) {
            *searchResult = RESULT_NOT_FOUND;
        }
        return _default;
    }

    char* end;
    // long is only 32 bits on windows so it isn't enough to detect int overflow
    long long val = strtoll(prop, &end, 10);
    if (val < INT_MIN || val > INT_MAX ||
        end == prop || *end != '\0') {
        D("Invalid int property: '%s:%s'", key, prop);
        AFREE(prop);
        if (searchResult) {
            *searchResult = RESULT_INVALID;
        }
        return _default;
    }

    AFREE(prop);

    if (searchResult) {
        *searchResult = RESULT_FOUND;
    }
    return (int)val;
}

int
propertyFile_getApiLevel(const FileData* data) {
    const int kMinLevel = 3;
    const int kMaxLevel = 10000;
    SearchResult searchResult;
    int level = propertyFile_getInt(data, "ro.build.version.sdk", kMinLevel,
                                    &searchResult);
    if (searchResult == RESULT_NOT_FOUND) {
        level = kMaxLevel;
        D("Could not find SDK version in build.prop, default is: %d", level);
    } else if (searchResult == RESULT_INVALID || level < 0) {
        D("Defaulting to target API sdkVersion %d", level);
    } else {
        D("Found target API sdkVersion: %d\n", level);
    }
    return level;
}

#define IN_PRODUCT_NAME(data, names) \
    propertyFile_findProductName( \
            data, names, ARRAY_SIZE(names), false /*prefix*/) \

AvdFlavor propertyFile_getAvdFlavor(const FileData* data) {
    AvdFlavor res = AVD_OTHER;

    const char* phone_names[] = {"phone"};
    const char* tv_names[] = {"atv"};
    const char* wear_names[] = {"aw"};
    const char* car_names[] = {"car_emu"};

    if (IN_PRODUCT_NAME(data, phone_names)) {
        res = AVD_PHONE;
    } else if (IN_PRODUCT_NAME(data, tv_names)) {
        res = AVD_TV;
    } else if (IN_PRODUCT_NAME(data, wear_names)) {
        res = AVD_WEAR;
    } else if (IN_PRODUCT_NAME(data, car_names)) {
        res = AVD_ANDROID_AUTO;
    }

    return res;
}

bool propertyFile_isGoogleApis(const FileData* data) {
    const char* google_names[] = {"sdk_google", "google_sdk"};
    return propertyFile_findProductName(
            data, google_names, ARRAY_SIZE(google_names), false /*prefix*/);
}

bool propertyFile_isUserBuild(const FileData* data) {
    bool isUser = false;
    char* prop = propertyFile_getValue((const char*)data->data, data->size,
                                       "ro.build.type");
    if (!prop) {
        return false;
    }
    if (!strcmp(prop, "user")) {
       isUser = true;
    }
    free(prop);
    return isUser;
}


bool propertyFile_findProductName(const FileData* data,
                                  const char* productNames[],
                                  int count,
                                  bool prefix) {
    char* prop = propertyFile_getValue((const char*)data->data, data->size,
                                       "ro.product.name");
    if (!prop) {
        return false;
    }
    if (!prefix) {
        int i;
        for (i = 0; i < count; i++) {
            if (strstr(prop, productNames[i])) {
                free(prop);
                return true;
            }
        }
    } else {
        int i;
        for (i = 0; i < count; i++) {
            int len = strlen(productNames[i]);
            if (strlen(prop) >= len &&
                strncmp(productNames[i], prop, len) == 0) {
                free(prop);
                return true;
            }
        }
    }

    free(prop);
    return false;
}

char* path_getBuildBuildProp(const char* androidOut) {
    char temp[MAX_PATH], *p = temp, *end = p + sizeof(temp);
    p = bufprint(temp, end, "%s/system/build.prop", androidOut);
    if (p >= end) {
        D("ANDROID_BUILD_OUT is too long: %s\n", androidOut);
        return NULL;
    }
    if (!path_exists(temp)) {
        D("Cannot find build properties file: %s\n", temp);
        return NULL;
    }
    return ASTRDUP(temp);
}


char* path_getBuildBootProp(const char* androidOut) {
    char temp[MAX_PATH], *p = temp, *end = p + sizeof(temp);
    p = bufprint(temp, end, "%s/boot.prop", androidOut);
    if (p >= end) {
        D("ANDROID_BUILD_OUT is too long: %s\n", androidOut);
        return NULL;
    }
    if (!path_exists(temp)) {
        D("Cannot find boot properties file: %s\n", temp);
        return NULL;
    }
    return ASTRDUP(temp);
}


char*
path_getBuildTargetArch(const char* androidOut) {
    char* buildPropPath = path_getBuildBuildProp(androidOut);
    if (!buildPropPath) {
        return NULL;
    }

    FileData buildProp[1];
    fileData_initFromFile(buildProp, buildPropPath);
    char* ret = propertyFile_getTargetArch(buildProp);
    fileData_done(buildProp);
    AFREE(buildPropPath);
    return ret;
}


static char*
_getAvdConfigValue(const char* avdPath,
                   const char* key,
                   const char* defaultValue)
{
    CIniFile* ini;
    char* result = NULL;
    char temp[PATH_MAX], *p = temp, *end = p + sizeof(temp);
    p = bufprint(temp, end, "%s" PATH_SEP CORE_CONFIG_INI, avdPath);
    if (p >= end) {
        APANIC("AVD path too long: %s\n", avdPath);
    }
    ini = iniFile_newFromFile(temp);
    if (ini == NULL) {
        APANIC("Could not open AVD config file: %s\n", temp);
    }
    result = iniFile_getString(ini, key, defaultValue);
    iniFile_free(ini);

    return result;
}

char*
path_getAvdTargetArch( const char* avdName )
{
    char*  avdPath = path_getAvdContentPath(avdName);
    char*  avdArch = _getAvdConfigValue(avdPath, "hw.cpu.arch", "arm");
    AFREE(avdPath);

    return avdArch;
}

char*
path_getAvdSnapshotPresent( const char* avdName )
{
    char*  avdPath = path_getAvdContentPath(avdName);
    char*  snapshotPresent = _getAvdConfigValue(avdPath, "snapshot.present", "no");
    AFREE(avdPath);

    return snapshotPresent;
}

char*
path_getAvdSystemPath(const char* avdName,
                      const char* sdkRoot) {
    char* result = NULL;
    char* avdPath = path_getAvdContentPath(avdName);
    int nn;
    for (nn = 0; nn < MAX_SEARCH_PATHS; ++nn) {
        char searchKey[32];
        snprintf(searchKey, sizeof(searchKey), "%s%d", SEARCH_PREFIX, nn + 1);
        char* searchPath = _getAvdConfigValue(avdPath, searchKey, NULL);
        if (!searchPath) {
            continue;
        }

        char temp[PATH_MAX], *p = temp, *end= p+sizeof temp;
        // Prefix sdkRoot if the path is not absolute
        if (path_is_absolute(searchPath)) {
          p = strncpy(temp, searchPath, sizeof(temp));
        } else {
          p = bufprint(temp, end, "%s/%s", sdkRoot, searchPath);
        }
        free(searchPath);
        if (p >= end || !path_is_dir(temp)) {
            D(" Not a directory: %s\n", temp);
            continue;
        }
        D(" Found directory: %s\n", temp);
        result = ASTRDUP(temp);
        break;
    }
    AFREE(avdPath);
    return result;
}

char*
path_getAvdGpuMode(const char* avdName)
{
    char* avdPath = path_getAvdContentPath(avdName);
    char* gpuEnabled = _getAvdConfigValue(avdPath, "hw.gpu.enabled", "no");
    bool enabled = !strcmp(gpuEnabled, "yes");
    AFREE(gpuEnabled);

    char* gpuMode = NULL;
    if (enabled) {
        gpuMode = _getAvdConfigValue(avdPath, "hw.gpu.mode", "auto");
    }
    AFREE(avdPath);
    return gpuMode;
}

char*
path_getAvdGpuBlacklisted(const char* avdName)
{
    char* avdPath = path_getAvdContentPath(avdName);
    char* gpuBlacklisted = NULL;
    gpuBlacklisted = _getAvdConfigValue(avdPath, "hw.gpu.blacklisted", "no");
    AFREE(avdPath);
    return gpuBlacklisted;
}

const char*
emulator_getBackendSuffix(const char* targetArch)
{
    if (!targetArch)
        return NULL;

    static const struct {
        const char* avd_arch;
        const char* emulator_suffix;
    } kPairs[] = {
        { "arm", "arm" },
        { "x86", "x86" },
        { "x86_64", "x86" },
        { "mips", "mips" },
        { "arm64", "arm64" },
        { "mips64", "mips64" },
        // Add more if needed here.
    };
    size_t n;
    for (n = 0; n < sizeof(kPairs)/sizeof(kPairs[0]); ++n) {
        if (!strcmp(targetArch, kPairs[n].avd_arch)) {
            return kPairs[n].emulator_suffix;
        }
    }
    return NULL;
}
