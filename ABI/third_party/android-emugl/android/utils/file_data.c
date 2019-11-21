// Copyright 2014 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/utils/file_data.h"

#include "android/utils/panic.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Use a magic value in the |flags| field to indicate that a FileData
// value was properly initialized. Helps catch errors at runtime.
#define FILE_DATA_MAGIC   ((size_t)0x87002013U)


bool fileData_isValid(const FileData* data) {
    if (!data)
        return false;
    if (data->flags == FILE_DATA_MAGIC)
        return true;
    if (data->flags == 0 && data->data == NULL && data->size == 0)
        return true;
    return false;
}

static inline void fileData_setValid(FileData* data) {
    data->flags = FILE_DATA_MAGIC;
}


static inline void fileData_setInvalid(FileData* data) {
    data->flags = (size_t)0xDEADBEEFU;
}


static void fileData_initWith(FileData* data,
                              const void* buff,
                              size_t size) {
    data->data = size ? (uint8_t*)buff : NULL;
    data->size = size;
    fileData_setValid(data);
}


void fileData_initEmpty(FileData* data) {
    fileData_initWith(data, NULL, 0);
}


int fileData_initFromFile(FileData* data, const char* filePath) {
    FILE* f = fopen(filePath, "rb");
    if (!f)
        return -errno;

    int ret = 0;
    do {
        if (fseek(f, 0, SEEK_END) < 0) {
            ret = -errno;
            break;
        }

        long fileSize = ftell(f);
        if (fileSize < 0) {
            ret = -errno;
            break;
        }

        if (fileSize == 0) {
            fileData_initEmpty(data);
            break;
        }

        if (fseek(f, 0, SEEK_SET) < 0) {
            ret = -errno;
            break;
        }

        char* buffer = malloc((size_t)fileSize);
        if (!buffer) {
            ret = -errno;
            break;
        }

        size_t readLen = fread(buffer, 1, (size_t)fileSize, f);
        if (readLen != (size_t)fileSize) {
            if (feof(f)) {
                ret = -EIO;
            } else {
                ret = -ferror(f);
            }
            break;
        }

        fileData_initWith(data, buffer, readLen);

    } while (0);

    fclose(f);
    return ret;
}


int fileData_initFrom(FileData* data, const FileData* other) {
    if (!other || !fileData_isValid(other)) {
        APANIC("Trying to copy an uninitialized FileData instance\n");
    }
    if (other->size == 0) {
        fileData_initEmpty(data);
        return 0;
    }
    void* copy = malloc(other->size);
    if (!copy) {
        return -errno;
    }

    memcpy(copy, other->data, other->size);
    fileData_initWith(data, copy, other->size);
    return 0;
}


int fileData_initFromMemory(FileData* data,
                             const void* input,
                             size_t inputLen) {
    FileData other;
    fileData_initWith(&other, input, inputLen);
    memset(data, 0, sizeof(*data));  // make valgrind happy.
    return fileData_initFrom(data, &other);
}


void fileData_swap(FileData* data, FileData* other) {
    if (!fileData_isValid(data) || !fileData_isValid(data))
        APANIC("Trying to swap un-initialized FileData instance\n");

    uint8_t* buffer = data->data;
    data->data = other->data;
    other->data = buffer;

    size_t size = data->size;
    data->size = other->size;
    other->size = size;
}


void fileData_done(FileData* data) {
    if (!fileData_isValid(data)) {
        APANIC("Trying to finalize an un-initialized FileData instance\n");
    }

    free(data->data);
    fileData_initWith(data, NULL, 0);
    fileData_setInvalid(data);
}
