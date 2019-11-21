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

#pragma once

#include "android/utils/compiler.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

ANDROID_BEGIN_HEADER

// A simple structure used to own a buffer of heap-allocated memory that
// typically comes from a file.
// |data| is the address of the corresponding data in memory.
// |size| is the size of the data in bytes.
// |flags| is used internally, do not use it.
// Note that |data| will always be NULL if |size| is 0.
typedef struct {
    uint8_t* data;
    size_t size;
    // private
    size_t flags;
} FileData;

// Initializer value for a FileData instance.
// Its important that this is all zeroes.
#define FILE_DATA_INIT  { NULL, 0, 0 }

// Return true iff a |fileData| is empty.
static inline bool fileData_isEmpty(const FileData* fileData) {
    return fileData->size == 0;
}

// Returns true iff |fileData| is valid. Used for unit-testing.
bool fileData_isValid(const FileData* fileData);

// Initialize a FileData value to the empty value.
void fileData_initEmpty(FileData* fileData);

// Initialize a FileData value by reading the content of a given file
// at |filePath|. On success, return 0 and initializes |fileData| properly.
// On failure, return -errno code, and set |fileData| to FILE_DATA_INIT.
int fileData_initFromFile(FileData* fileData, const char* filePath);

// Initialize a FileData by copying a memory buffer.
// |fileData| is the address of the FileData value to initialize.
// |buffer| is the address of the input buffer that will be copied
// into the FileData.
// |bufferLen| is the buffer length in bytes.
// Return 0 on success, -errno code on failure.
int fileData_initFromMemory(FileData* fileData,
                            const void* buffer,
                            size_t bufferLen);

// Copy a FileData value into another one. This copies the contents in
// the heap. On success return 0, on failure -errno code.
int fileData_initFrom(FileData* fileData, const FileData* other);

// Swap two FileData values.
void fileData_swap(FileData* fileData, FileData* other);

// Finalize a FileData value. This releases the corresponding memory.
void fileData_done(FileData* fileData);

ANDROID_END_HEADER
