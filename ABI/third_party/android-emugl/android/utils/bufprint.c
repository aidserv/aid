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

#include "android/utils/bufprint.h"
#include "android/utils/path.h"
#include "android/utils/debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include "windows.h"
#  include "shlobj.h"
#else
#  include <unistd.h>
#  include <sys/stat.h>
#endif

#define  D(...)  VERBOSE_PRINT(init,__VA_ARGS__)


/** USEFUL STRING BUFFER FUNCTIONS
 **/

char*
vbufprint( char*        buffer,
           char*        buffer_end,
           const char*  fmt,
           va_list      args )
{
    int  len = vsnprintf( buffer, buffer_end - buffer, fmt, args );
    if (len < 0 || buffer+len >= buffer_end) {
        if (buffer < buffer_end)
            buffer_end[-1] = 0;
        return buffer_end;
    }
    return buffer + len;
}

char*
bufprint(char*  buffer, char*  end, const char*  fmt, ... )
{
    va_list  args;
    char*    result;

    va_start(args, fmt);
    result = vbufprint(buffer, end, fmt, args);
    va_end(args);
    return  result;
}
