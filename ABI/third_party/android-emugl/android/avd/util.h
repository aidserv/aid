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

#pragma once

#include "android/utils/compiler.h"
#include "android/utils/file_data.h"

ANDROID_BEGIN_HEADER

/* A collection of simple functions to extract relevant AVD-related
 * information either from an SDK AVD or a platform build.
 */

/* Return the path to the Android SDK root installation.
 * Caller must free() returned string.
 */
char* path_getSdkRoot();

/* Return the path to the AVD's root configuration .ini file. it is located in
 * ~/.android/avd/<name>.ini or Windows equivalent
 *
 * This file contains the path to the AVD's content directory, which
 * includes its own config.ini.
 */
char* path_getRootIniPath( const char*  avdName );

/* Return the path to the AVD's content directory. it is located in
 *  ~/.android/avd/<name>.avd or Windows equivalent
 * Caller must free() returned string.
 */
char* path_getAvdContentPath( const char* avdName );

/* Return the target architecture for a given AVD.
 * Caller must free() returned string.
 */
char* path_getAvdTargetArch( const char* avdName );

/* Return the snapshot.present for a given AVD.
 * Caller must free() returned string.
 */
char* path_getAvdSnapshotPresent( const char* avdName );

/* Return the path to an AVD's system images directory, relative to a given
 * root SDK path. Caller must free() the result.
 */
char* path_getAvdSystemPath(const char* avdName,
                            const char* sdkRootPath);

/* Return the value of hw.gpu.mode for a given AVD.
 * Caller must free() returned string.
 *
 * NOTE: If hw.gpu.enabled is false, then this returns NULL
 */
char* path_getAvdGpuMode(const char* avdName);

/* Return the value of hw.gpu.blacklisted for a given AVD.
 * Caller must free() returned string.
 */
char* path_getAvdGpuBlacklisted(const char* avdName);

typedef enum {
    RESULT_INVALID   = -1, // key was found but value contained invalid data
    RESULT_FOUND     =  0, // key was found and value parsed correctly
    RESULT_NOT_FOUND =  1, // key was not found (default used)
} SearchResult;

/* Retrieves an integer value associated with the key parameter
 *
 * |data| is a FileData instance
 * |key| name of key to search for
 * |searchResult| if non-null, this is set to RESULT_INVALID, RESULT_FOUND,
 *                or RESULT_NOT_FOUND
 * Returns valid parsed int value if found, |default| otherwise
 */
int propertyFile_getInt(const FileData* data, const char* key, int _default,
                        SearchResult* searchResult);

/* Retrieves a string corresponding to the target architecture
 * extracted from a build properties file.
 *
 * |data| is a FileData instance holding the build.prop contents.
 * Returns a a new string that must be freed by the caller, which can
 * be 'armeabi', 'armeabi-v7a', 'x86', etc... or NULL if
 * it cannot be determined.
 */
char* propertyFile_getTargetAbi(const FileData* data);

/* Retrieves a string corresponding to the target architecture
 * extracted from a build properties file.
 *
 * |data| is a FileData instance holding the build.prop contents.
 * Returns a new string that must be freed by the caller, which can
 * be 'arm', 'x86, 'mips', etc..., or NULL if if cannot be determined.
 */
char* propertyFile_getTargetArch(const FileData* data);

/* Retrieve the target API level from the build.prop contents.
 * Returns a very large value (e.g. 100000) if it cannot be determined
 * (which happens for platform builds), or 3 (the minimum SDK API level)
 * if there is invalid value.
 */
int propertyFile_getApiLevel(const FileData* data);

typedef enum {
    AVD_PHONE = 0,
    AVD_TV = 1,
    AVD_WEAR = 2,
    AVD_ANDROID_AUTO = 3,
    AVD_OTHER = 255,
} AvdFlavor;

/* Determine the flavor of system image. */
AvdFlavor propertyFile_getAvdFlavor(const FileData* data);

/* Determine whether a Google API's system image is used. */
bool propertyFile_isGoogleApis(const FileData* data);

/* Determine whether the system image is a user build. */
bool propertyFile_isUserBuild(const FileData* data);

/* Return whether the product name in property file has a substring matching any
 * of the the given ones.
 *
 * |productNames| is a list of product names to compare to.
 * |len| is the number of product names.
 * |prefix| is whether the given product name needs to be the prefix.
 */
bool propertyFile_findProductName(const FileData* data,
                                  const char *productNames[],
                                  int len,
                                  bool prefix);

/* Retrieve the mode describing how the ADB daemon is communicating with
 * the emulator from inside the guest.
 * Return 0 for legacy mode, which uses TCP port 5555.
 * Return 1 for the 'qemud' mode, which uses a QEMUD service instead.
 */
int propertyFile_getAdbdCommunicationMode(const FileData* data);

/* Return the path of the build properties file (build.prop) from an
 * Android platform build, or NULL if it doesn't exist.
 */
char* path_getBuildBuildProp( const char* androidOut );

/* Return the path of the boot properties file (boot.prop) from an
 * Android platform build, or NULL if it doesn't exit.
 */
char* path_getBuildBootProp( const char* androidOut );

/* Return the target architecture from the build properties file
 *  (build.prop) of an Android platformn build. Return NULL or a new
 * string that must be freed by the caller.
 */
char* path_getBuildTargetArch( const char* androidOut );

/* Given an AVD's target architecture, return the suffix of the
 * corresponding emulator backend program, e.g. 'x86' for x86 and x86_64
 * CPUs, 'arm' for ARM ones (including ARM64 when support is complete),
 * etc. Returned string must not be freed by the caller.
 */
const char* emulator_getBackendSuffix(const char* targetArch);

/* this is the subdirectory of $HOME/.android where all
 * root configuration files (and default content directories)
 * are located.
 */
#define  ANDROID_AVD_DIR    "avd"

ANDROID_END_HEADER
