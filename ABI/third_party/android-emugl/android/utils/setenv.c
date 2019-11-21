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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
int setenv(const char *name, const char *value, int overwrite)
{
    int result = 0;
    if (overwrite || !getenv(name)) {
        size_t length = strlen(name) + strlen(value) + 2;
        char *string = malloc(length);
        snprintf(string, length, "%s=%s", name, value);
        result = putenv(string);
    }
    return result;
}

#endif
