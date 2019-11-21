/* Copyright (C) 2012 The Android Open Source Project
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

/* Keys of the properties found in avd/name.ini and config.ini files.
 *
 * These keys must match their counterpart defined in
 * sdk/sdkmanager/libs/sdklib/src/com/android/sdklib/internal/avd/AvdManager.java
 */


/* -- Keys used in avd/name.ini -- */

/* Absolute path of the AVD content directory.
 */
#define  ROOT_ABS_PATH_KEY    "path"

/* Relative path of the AVD content directory.
 * Path is relative to the bufprint_config_path().
 */
#define  ROOT_REL_PATH_KEY    "path.rel"


/* -- Keys used in config.ini -- */

/* AVD/config.ini key name representing the abi type of the specific avd
 */
#define ABI_TYPE "abi.type"

/* AVD/config.ini key name representing the CPU architecture of the specific avd
 */
#define CPU_ARCH "hw.cpu.arch"
/* the prefix of config.ini keys that will be used for search directories
 * of system images.
 */
#define  SEARCH_PREFIX   "image.sysdir."

/* the maximum number of search path keys we're going to read from the
 * config.ini file
 */
#define  MAX_SEARCH_PATHS  2

/* the config.ini key that will be used to indicate the full relative
 * path to the skin directory (including the skin name).
 */
#define  SKIN_PATH       "skin.path"

/* the config.ini key that will be used to indicate the default skin's name.
 * this is ignored if there is a valid SKIN_PATH entry in the file.
 */
#define  SKIN_NAME       "skin.name"

/* default skin name */
#define  SKIN_DEFAULT    "HVGA"

/* the config.ini key that is used to indicate the absolute path
 * to the SD Card image file, if you don't want to place it in
 * the content directory.
 */
#define  SDCARD_PATH     "sdcard.path"

/* The config.ini key name representing the second path where the emulator looks
 * for system images. Typically this is the path to the platform system image.
 */
#define IMAGES_2 "image.sysdir.2"

/* AVD/config.ini key name representing the presence of the snapshots file.
 */
#define SNAPSHOT_PRESENT "snapshot.present"

/* AVD/config.ini key name representing the size of the SD card.
 */
#define SDCARD_SIZE "sdcard.size"

/* AVD/config.ini key name representing the tag id of the specific avd
 */
#define TAG_ID "tag.id"

/* AVD/config.ini key name representing the tag display of the specific avd
 */
#define TAG_DISPLAY "tag.display"
