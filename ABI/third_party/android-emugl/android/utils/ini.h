/* Copyright (C) 2008-2015 The Android Open Source Project
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

#include <stdbool.h>
#include <stdint.h>

#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

/* the emulator supports a simple .ini file format for its configuration
 * files. See docs/ANDROID-CONFIG-FILES.TXT for details.
 */

/* an opaque structure used to model an .ini configuration file */
typedef struct CIniFile CIniFile;

/*
 * Creates a new empty CIniFile not bound to any underlying file, and without
 * any
 * data. |filePath| is only used when writing a warning to stderr, in case of
 * badly formed output.
 */
CIniFile* iniFile_newEmpty(const char* filePath);

/* creates a new CIniFile object from a file path,
 * returns NULL if the file cannot be opened.
 */
CIniFile* iniFile_newFromFile(const char* filePath);

/* try to write an CIniFile into a given file.
 * returns 0 on success, -1 on error (see errno for error code)
 */
int iniFile_saveToFile(CIniFile* f, const char* filePath);

/* try to write an CIniFile into a given file, ignorig pairs with empty values.
 * returns 0 on success, -1 on error (see errno for error code)
 */
int iniFile_saveToFileClean(CIniFile* f, const char* filepath);

/* free an CIniFile object */
void iniFile_free(CIniFile* f);

/* returns the number of (key.value) pairs in an CIniFile */
int iniFile_getPairCount(CIniFile* f);

/* Check if a key exists in the iniFile */
bool iniFile_hasKey(CIniFile* f, const char* key);

/* Copies a 'key, value' pair for an entry in the file.
 * Param:
 *  f - Initialized CIniFile instance.
 *  index - Index of the entry to copy. Must be less than value returned from
 * the
 *      iniFile_getPairCount routine.
 *  key, value - Receives key, and value strings for the entry. If this routine
 *      succeeds, the caller must free the buffers allocated for the strings.
 * Return:
 *  0 on success, -1 if the index exceeds the capacity of the file
 */
int iniFile_getEntry(CIniFile* f, int index, char** key, char** value);

/* returns a copy of the value of a given key, or NULL if defaultValue is NULL.
 * caller must free() it.
 */
char* iniFile_getString(CIniFile* f, const char* key, const char* defaultValue);

/* returns an integer value, or a default in case the value string is
 * missing or badly formatted
 */
int iniFile_getInteger(CIniFile* f, const char* key, int defaultValue);

/* returns a 64-bit integer value, or a default in case the value string is
 * missing or badly formatted
 */
int64_t iniFile_getInt64(CIniFile* f, const char* key, int64_t defaultValue);

/* returns a double value, or a default in case the value string is
 * missing or badly formatted
 */
double iniFile_getDouble(CIniFile* f, const char* key, double defaultValue);

/* parses a key value as a boolean. Accepted values are "1", "0", "yes", "YES",
 * "no" and "NO". Returns either 1 or 0.
 * note that the default value must be provided as a string too
 */
int iniFile_getBoolean(CIniFile* f, const char* key, const char* defaultValue);

/* parses a key value as a disk size. this means it can be an integer followed
 * by a suffix that can be one of "mMkKgG" which correspond to KiB, MiB and GiB
 * multipliers.
 *
 * NOTE: we consider that 1K = 1024, not 1000.
 */
int64_t iniFile_getDiskSize(CIniFile* f,
                            const char* key,
                            const char* defaultValue);

/* These functions are used to set values in an CIniFile */
void iniFile_setValue(CIniFile* f, const char* key, const char* value);
void iniFile_setInteger(CIniFile* f, const char* key, int value);
void iniFile_setInt64(CIniFile* f, const char* key, int64_t value);
void iniFile_setDouble(CIniFile* f, const char* key, double value);
void iniFile_setBoolean(CIniFile* f, const char* key, int value);
void iniFile_setDiskSize(CIniFile* f, const char* key, int64_t size);
void iniFile_setString(CIniFile* f, const char* key, const char* str);

ANDROID_END_HEADER
