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
#include "android/skin/resource.h"
#include <string.h>

typedef struct {
    const char*           name;
    const unsigned char*  base;
    size_t                size;
} FileEntry;

static const unsigned char* _resource_find(
        const char* name,
        const FileEntry* entries,
        size_t* psize) {
    const FileEntry* e = entries;
    for ( ; e->name != NULL; e++ ) {
        if ( strcmp(e->name, name) == 0 ) {
            *psize = e->size;
            return e->base;
        }
    }
    return NULL;
}

#undef   _file_entries
#define  _file_entries  _skin_entries
const unsigned char* skin_resource_find(const char* name,
                                        size_t* psize) {
#    include "android/skin/resource_defaults.h"
    return _resource_find( name, _file_entries, psize );
}
