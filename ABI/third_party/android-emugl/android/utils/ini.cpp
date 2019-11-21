// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/utils/ini.h"

#include "android/base/files/IniFile.h"
#include "android/base/Log.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>

typedef ::CIniFile CIniFile;
typedef android::base::IniFile BaseIniFile;

static BaseIniFile* asBaseIniFile(CIniFile* i) {
    return reinterpret_cast<BaseIniFile*>(i);
}

void iniFile_free(CIniFile* i) {
    delete asBaseIniFile(i);
}

bool iniFile_hasKey(CIniFile* i, const char* key) {
    return asBaseIniFile(i)->hasKey(key);
}

int iniFile_getPairCount(CIniFile* i) {
    return asBaseIniFile(i)->size();
}

CIniFile* iniFile_newEmpty(const char* filepath) {
    return reinterpret_cast<CIniFile*>(new BaseIniFile());
}

CIniFile* iniFile_newFromFile(const char* filepath) {
    BaseIniFile* ini = new BaseIniFile(filepath);
    ini->read();
    return reinterpret_cast<CIniFile*>(ini);
}

// NB: Returns 0 on success.
int iniFile_saveToFile(CIniFile* f, const char* filepath) {
    asBaseIniFile(f)->setBackingFile(filepath);
    return !asBaseIniFile(f)->write();
}

// NB: Returns 0 on success.
int iniFile_saveToFileClean(CIniFile* f, const char* filepath) {
    asBaseIniFile(f)->setBackingFile(filepath);
    return !asBaseIniFile(f)->writeDiscardingEmpty();
}

int iniFile_getEntry(CIniFile* f, int index, char** key, char** value) {
    BaseIniFile* ini = asBaseIniFile(f);
    if (index >= ini->size()) {
        LOG(VERBOSE) << "Index " << index
                     << "exceeds the number of ini file entries "
                     << ini->size();
        return -1;
    }

    BaseIniFile::const_iterator cit = ini->begin();
    for (; index > 0; --index) {
        ++cit;
    }
    *key = strdup(cit->c_str());
    *value = strdup(ini->getString(*cit, "").c_str());
    return 0;
}

char* iniFile_getString(CIniFile* f,
                        const char* key,
                        const char* defaultValue) {
    if (defaultValue != NULL ) {
        return strdup(asBaseIniFile(f)->getString(key, defaultValue).c_str());
    }
    // |defaultValue| of NULL is not handled well by getString.
    if (!asBaseIniFile(f)->hasKey(key)) {
        return NULL;
    }
    return strdup(asBaseIniFile(f)->getString(key, "").c_str());
}

int iniFile_getInteger(CIniFile* f, const char* key, int defaultValue) {
    return asBaseIniFile(f)->getInt(key, defaultValue);
}

double iniFile_getDouble(CIniFile* f, const char* key, double defaultValue) {
    return asBaseIniFile(f)->getDouble(key, defaultValue);
}

int iniFile_getBoolean(CIniFile* f, const char* key, const char* defaultValue) {
    return asBaseIniFile(f)->getBool(key, defaultValue);
}

int64_t iniFile_getDiskSize(CIniFile* f,
                            const char* key,
                            const char* defaultValue) {
    return asBaseIniFile(f)->getDiskSize(key, defaultValue);
}

int64_t iniFile_getInt64(CIniFile* f, const char* key, int64_t defaultValue) {
    return asBaseIniFile(f)->getInt64(key, defaultValue);
}

void iniFile_setValue(CIniFile* f, const char* key, const char* value) {
    // Silently protect us from blowing up.
    asBaseIniFile(f)->setString(key, (value == NULL) ? "" : value);
}

void iniFile_setInteger(CIniFile* f, const char* key, int value) {
    asBaseIniFile(f)->setInt(key, value);
}

void iniFile_setInt64(CIniFile* f, const char* key, int64_t value) {
    asBaseIniFile(f)->setInt64(key, value);
}

void iniFile_setDouble(CIniFile* f, const char* key, double value) {
    asBaseIniFile(f)->setDouble(key, value);
}

void iniFile_setBoolean(CIniFile* f, const char* key, int value) {
    asBaseIniFile(f)->setBool(key, value);
}

void iniFile_setDiskSize(CIniFile* f, const char* key, int64_t size) {
    asBaseIniFile(f)->setDiskSize(key, size);
}

void iniFile_setString(CIniFile* f, const char* key, const char* str) {
    asBaseIniFile(f)->setString(key, str);
}
