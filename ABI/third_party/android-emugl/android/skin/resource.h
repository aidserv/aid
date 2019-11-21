/* Copyright (C) 2007-2008 The Android Open Source Project
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

#include <stddef.h>

// Find the resource data identified by |name|. On success return its
// first address in memory, and sets |*psize| to its size in bytes.
// On failure, return NULL.
const unsigned char* skin_resource_find(const char* name,
                                        size_t* psize );
