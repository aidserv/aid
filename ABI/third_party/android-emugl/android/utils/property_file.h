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

#include <stdbool.h>
#include <stddef.h>

#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

// Parse the content of a property file and retrieve the value of a given
// named property, or NULL if it is undefined or empty. If a property
// appears several times in a file, the last definition is returned.
// |propertyFile| is the address of the file in memory.
// |propertyFileLen| is its length in bytes.
// |propertyName| is the name of the property.
char* propertyFile_getValue(const char* propertyFile,
                            size_t propertyFileLen,
                            const char* propertyName);

// Maximum length of a property name (including terminating zero).
// Any property name that is equal or greater than this value will be
// considered undefined / ignored.
#define MAX_PROPERTY_NAME_LEN  32

// Maximum length of a property value (including terminating zero).
// Any value stored in a file that has a length equal or greater than this
// will be truncated!.
#define MAX_PROPERTY_VALUE_LEN 92

// Structure used to hold an iterator over a property file.
// Usage is simple:
//    1) Initialize iterator with propertyFileIterator_init()
//
//    2) Call propertyFileIterator_next() in a loop. If it returns true
//       one can read the |name| and |value| zero-terminated strings to
//       get property names and values, in the order they appear in the
//       file.
//
//       Once propertyFileIterator_next() returns false, you're done.
//
typedef struct {
    char name[MAX_PROPERTY_NAME_LEN];
    char value[MAX_PROPERTY_VALUE_LEN];
    // private.
    const char* p;
    const char* end;
} PropertyFileIterator;

// Initialize a PropertyFileIterator.
// |iter| is the iterator instance.
// |propertyFile| is the address of the property file in memory.
// |propertyFileLen| is its lengh in bytes.
void propertyFileIterator_init(PropertyFileIterator* iter,
                               const void* propertyFile,
                               size_t propertyFileLen);

// Extract one property from a property file iterator.
// Returns true if there is one, or false if the iteration has stopped.
// If true, one can read |iter->name| and |iter->value| to get the
// property name and value, respectively, as zero-terminated strings
// that need to be copied by the caller.
bool propertyFileIterator_next(PropertyFileIterator* iter);

ANDROID_END_HEADER
