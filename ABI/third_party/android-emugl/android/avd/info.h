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

#pragma once

#include "android/avd/hw-config.h"
#include "android/avd/util.h"
#include "android/utils/compiler.h"
#include "android/utils/file_data.h"
#include "android/utils/ini.h"

ANDROID_BEGIN_HEADER

/* An Android Virtual Device (AVD for short) corresponds to a
 * directory containing all kernel/disk images for a given virtual
 * device, as well as information about its hardware capabilities,
 * SDK version number, skin, etc...
 *
 * Each AVD has a human-readable name and is backed by a root
 * configuration file and a content directory. For example, an
 *  AVD named 'foo' will correspond to the following:
 *
 *  - a root configuration file named ~/.android/avd/foo.ini
 *    describing where the AVD's content can be found
 *
 *  - a content directory like ~/.android/avd/foo/ containing all
 *    disk image and configuration files for the virtual device.
 *
 * the 'foo.ini' file should contain at least one line of the form:
 *
 *    rootPath=<content-path>
 *
 * it may also contain other lines that cache stuff found in the
 * content directory, like hardware properties or SDK version number.
 *
 * it is possible to move the content directory by updating the foo.ini
 * file to point to the new location. This can be interesting when your
 * $HOME directory is located on a network share or in a roaming profile
 * (Windows), given that the content directory of a single virtual device
 * can easily use more than 100MB of data.
 *
 */


/* a macro used to define the list of disk images managed by the
 * implementation. This macro will be expanded several times with
 * varying definitions of _AVD_IMG
 */
#define  AVD_IMAGE_LIST \
    _AVD_IMG(KERNEL,"kernel-qemu","kernel") \
    _AVD_IMG(KERNELRANCHU,"kernel-ranchu","kernel") \
    _AVD_IMG(RAMDISK,"ramdisk.img","ramdisk") \
    _AVD_IMG(INITSYSTEM,"system.img","init system") \
    _AVD_IMG(INITDATA,"userdata.img","init data") \
    _AVD_IMG(INITZIP,"data","init data zip") \
    _AVD_IMG(USERSYSTEM,"system-qemu.img","user system") \
    _AVD_IMG(USERDATA,"userdata-qemu.img", "user data") \
    _AVD_IMG(CACHE,"cache.img","cache") \
    _AVD_IMG(SDCARD,"sdcard.img","SD Card") \
    _AVD_IMG(ENCRYPTIONKEY,"encryptionkey.img","Encryption Key") \
    _AVD_IMG(SNAPSHOTS,"snapshots.img","snapshots") \

/* define the enumared values corresponding to each AVD image type
 * examples are: AVD_IMAGE_KERNEL, AVD_IMAGE_SYSTEM, etc..
 */
#define _AVD_IMG(x,y,z)   AVD_IMAGE_##x ,
typedef enum {
    AVD_IMAGE_LIST
    AVD_IMAGE_MAX /* do not remove */
} AvdImageType;
#undef  _AVD_IMG

/* AvdInfo is an opaque structure used to model the information
 * corresponding to a given AVD instance
 */
typedef struct AvdInfo  AvdInfo;

/* various flags used when creating an AvdInfo object */
typedef enum {
    /* use to force a data wipe */
    AVDINFO_WIPE_DATA = (1 << 0),
    /* use to ignore the cache partition */
    AVDINFO_NO_CACHE  = (1 << 1),
    /* use to wipe cache partition, ignored if NO_CACHE is set */
    AVDINFO_WIPE_CACHE = (1 << 2),
    /* use to ignore ignore SDCard image (default or provided) */
    AVDINFO_NO_SDCARD = (1 << 3),
    /* use to wipe the system image with new initial values */
    AVDINFO_WIPE_SYSTEM = (1 << 4),
    /* use to ignore ignore state snapshot image (default or provided) */
    AVDINFO_NO_SNAPSHOTS = (1 << 5),
} AvdFlags;

typedef struct {
    unsigned     flags;
    const char*  skinName;
    const char*  skinRootPath;
    const char*  forcePaths[AVD_IMAGE_MAX];
} AvdInfoParams;

/* Creates a new AvdInfo object from a name. Returns NULL if name is NULL
 * or contains characters that are not part of the following list:
 * letters, digits, underscores, dashes and periods
 */
AvdInfo*  avdInfo_new( const char*  name, AvdInfoParams*  params );

/* Update the AvdInfo hardware config from a given skin name and path */
int avdInfo_getSkinHardwareIni( AvdInfo* i, char* skinName, char* skinDirPath);

/* A special function used to setup an AvdInfo for use when starting
 * the emulator from the Android build system. In this specific instance
 * we're going to create temporary files to hold all writable image
 * files, and activate all hardware features by default
 *
 * 'androidBuildRoot' must be the absolute path to the root of the
 * Android build system (i.e. the 'android' directory)
 *
 * 'androidOut' must be the target-specific out directory where
 * disk images will be looked for.
 */
AvdInfo*  avdInfo_newForAndroidBuild( const char*     androidBuildRoot,
                                      const char*     androidOut,
                                      AvdInfoParams*  params );

/* Frees an AvdInfo object and the corresponding strings that may be
 * returned by its getXXX() methods
 */
void        avdInfo_free( AvdInfo*  i );

/* Return the name of the Android Virtual Device
 */
const char*  avdInfo_getName( const AvdInfo*  i );

/* Return the target API level for this AVD.
 * Note that this will be some ridiculously large
 * value (e.g. 1000) if this value cannot be properly
 * determined (e.g. you're using an AVD from a preview SDK)
 */
int    avdInfo_getApiLevel( const AvdInfo*  i );

/* Return the "dessert name" associated with the
 * API level
 */
const char* avdInfo_getApiDessertName(int apiLevel);

/* Return the full version name associated with the
 * API level
 */
void avdInfo_getFullApiName(int apiLevel, char* nameStr, int strLen);

/* Return the api level corresponding to a single-letter API code,
 * e.g. 'N' -> 24
 */
int avdInfo_getApiLevelFromLetter(char letter);

/* Returns true if the AVD is on Google APIs. */
bool   avdInfo_isGoogleApis( const AvdInfo*  i );

/* Returns true if the AVD is a user build. */
bool avdInfo_isUserBuild(const AvdInfo* i);

/* Querying AVD flavors. */
AvdFlavor avdInfo_getAvdFlavor( const AvdInfo* i);

/* Returns the path to various images corresponding to a given AVD.
 * NULL if the image cannot be found. Returned strings must be freed
 * by the caller.
 */
char*  avdInfo_getKernelPath( const AvdInfo*  i );
char*  avdInfo_getRanchuKernelPath( const AvdInfo*  i );
char*  avdInfo_getRamdiskPath( const AvdInfo*  i );
char*  avdInfo_getSdCardPath( const AvdInfo* i );
char*  avdInfo_getEncryptionKeyImagePath( const AvdInfo* i );
char*  avdInfo_getSnapStoragePath( const AvdInfo* i );

/* This function returns NULL if the cache image file cannot be found.
 * Use avdInfo_getDefaultCachePath() to retrieve the default path
 * if you intend to create the partition file there.
 */
char*  avdInfo_getCachePath( const AvdInfo*  i );
char*  avdInfo_getDefaultCachePath( const AvdInfo*  i );

/* avdInfo_getSystemImagePath() will return NULL, except if the AVD content
 * directory contains a file named "system-qemu.img".
 */
char*  avdInfo_getSystemImagePath( const AvdInfo* i );

/* avdInfo_getSystemInitImagePath() retrieves the path to the read-only
 * initialization image for this disk image.
 */
char*  avdInfo_getSystemInitImagePath( const AvdInfo*  i );

char*  avdInfo_getDataImagePath( const AvdInfo*  i );
char*  avdInfo_getDefaultDataImagePath( const AvdInfo*  i );

/* avdInfo_getDefaultSystemFeatureControlPath() returns the path to the
 * per-system image feature control file
 */
char* avdInfo_getDefaultSystemFeatureControlPath(const AvdInfo* i);

/* avdInfo_getDataInitImagePath returns the path of userdata.img used
 * for initialization when -wipe-data.
 * Newer versions of system images shall use avdInfo_getDataInitDirPath
 * to generate userdata.img when -wipe-data
 */
char*  avdInfo_getDataInitImagePath( const AvdInfo* i );

/* avdInfo_getDataInitDirPath returns the path to the unpacked user data
 * folder. Newer versions of system images shall use the unpacked user data
 * instead of the prebuilt image to initialize userdata.img.
 */
char*  avdInfo_getDataInitDirPath( const AvdInfo* i );

/* Return a reference to the boot.prop file for this AVD, if any.
 * The file contains additionnal properties to inject at boot time
 * into the guest system. Note that this never returns NULL, but
 * the corresponding content can be empty.
 */
const FileData* avdInfo_getBootProperties(const AvdInfo* i);

/* Return a reference to the build.prop file for this AVD, if any.
 * The file contains information about the system image used in the AVD.
 * Note that this never returns NULL, but the corresponding content can
 * be empty.
 */
const FileData* avdInfo_getBuildProperties(const AvdInfo* i);

/* Return a reference to the avd/confif.init file for this AVD.
 * Note that this vaue could be NULL.
 */
CIniFile* avdInfo_getConfigIni(const AvdInfo* i);

/* Returns the path to a given AVD image file. This will return NULL if
 * the file cannot be found / does not exist.
 */
const char*  avdInfo_getImagePath( const AvdInfo*  i, AvdImageType  imageType );

/* Returns the default path of a given AVD image file. This only makes sense
 * if avdInfo_getImagePath() returned NULL.
 */
const char*  avdInfo_getImageDefaultPath( const AvdInfo*  i, AvdImageType  imageType );


/* Try to find the path of a given image file, returns NULL
 * if the corresponding file could not be found. the string
 * belongs to the AvdInfo object.
 */
const char*  avdInfo_getImageFile( const AvdInfo*  i, AvdImageType  imageType );

/* Return the size of a given image file. Returns 0 if the file
 * does not exist or could not be accessed.
 */
uint64_t     avdInfo_getImageFileSize( const AvdInfo*  i, AvdImageType  imageType );

/* Returns 1 if the corresponding image file is read-only
 */
int          avdInfo_isImageReadOnly( const AvdInfo*  i, AvdImageType  imageType );

/* lock an image file if it is writable. returns 0 on success, or -1
 * otherwise. note that if the file is read-only, it doesn't need to
 * be locked and the function will return success.
 */
int          avdInfo_lockImageFile( const AvdInfo*  i, AvdImageType  imageType, int  abortOnError);

/* Manually set the path of a given image file. */
void         avdInfo_setImageFile( const AvdInfo*  i, AvdImageType  imageType, const char*  imagePath );

/* Returns the content path of the virtual device */
const char*  avdInfo_getContentPath( const AvdInfo*  i );

/* Returns the root ini path of the virtual device */
const char*  avdInfo_getRootIniPath( const AvdInfo*  i );

/* Retrieve the AVD's specific skin information.
 * On exit:
 *   '*pSkinName' points to the skin's name.
 *   '*pSkinDir' points to the skin's directory.
 *
 * Note that the skin's content will be under <skinDir>/<skinName>.
 */
void         avdInfo_getSkinInfo( const AvdInfo*  i, char** pSkinName, char** pSkinDir );

/* Find a charmap file named <charmapName>.kcm for this AVD.
 * Returns the path of the file on success, or NULL if not found.
 * The result string must be freed by the caller.
 */
char*        avdInfo_getCharmapFile( const AvdInfo* i, const char* charmapName );

/* Returns TRUE iff in the Android build system */
int          avdInfo_inAndroidBuild( const AvdInfo*  i );

/* Return the target CPU architecture for this AVD.
 * This returns NULL if that cannot be determined, or a string that
 * must be freed by the caller otherwise.
 */
char*        avdInfo_getTargetCpuArch(const AvdInfo* i);

/* Returns the target ABI for the corresponding platform image.
 * This may return NULL if it cannot be determined. Otherwise this is
 * a string like "armeabi", "armeabi-v7a" or "x86" that must be freed
 * by the caller.
 */
char*        avdInfo_getTargetAbi( const AvdInfo*  i );

/* Reads the AVD's hardware configuration into 'hw'. returns -1 on error, 0 otherwise */
int          avdInfo_initHwConfig(const AvdInfo* i,
                                  AndroidHwConfig* hw,
                                  bool isQemu2);

/* Returns a *copy* of the path used to store profile 'foo'. result must be freed by caller */
char*        avdInfo_getCodeProfilePath( const AvdInfo*  i, const char*  profileName );

/* Returns the path of the hardware.ini where we will write the AVD's
 * complete hardware configuration before launching the corresponding
 * core.
 */
const char*  avdInfo_getCoreHwIniPath( const AvdInfo* i );

/* Describes the ways emulator may set up host adb <-> guest adbd communication:
 *  legacy - ADBD communicates with the emulator via forwarded TCP port 5555.
 *  pipe   - ADBD communicates with the emulator via 'adb' pipe service.
 */
typedef enum {
    ADBD_COMMUNICATION_MODE_LEGACY,
    ADBD_COMMUNICATION_MODE_PIPE,
} AdbdCommunicationMode;

/* Returns mode in which ADB daemon running in the guest communicates with the
 * emulator.
 */
AdbdCommunicationMode avdInfo_getAdbdCommunicationMode(const AvdInfo* i,
                                                       bool isQemu2);

/* Returns config.ini snapshot presense status.
 * This routine checks if snapshots are enabled in AVD config.ini file.
 * Return:
 *  1 - Snapshots are enabled in AVD config.ini file.
 *  0 - Snapshots are disabled in AVD config.ini file, of config.ini file is not
 *      found.
*/
int          avdInfo_getSnapshotPresent(const AvdInfo* i);

/* Returns the incremental version number of the AVD's system image.
 */
int avdInfo_getSysImgIncrementalVersion(const AvdInfo* i);

/* Return the tag of a specific AVD in the format of "|tag.id|[|tag.display|]".
 *  The default value is "default [Default]". The implementation is in reference
 * to
 * sdk/sdkmanager/libs/sdklib/src/com/android/sdklib/internal/avd/AvdInfo.java
 */
const char* avdInfo_getTag(const AvdInfo* i);

/* Return SD card size in human readable format or NULL if the SD card doesn't
 * exist.
 */
const char* avdInfo_getSdCardSize(const AvdInfo* i);

/* return 1 if it is build environment and encryption is enabled */
/* 0 otherwise */

int avdInfo_isEncryptionEnabledInBuild(const AvdInfo* i);

ANDROID_END_HEADER
