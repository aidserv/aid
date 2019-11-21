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

#include "android/utils/compiler.h"

#include <stdarg.h>

ANDROID_BEGIN_HEADER

// WARNING: Usage of bufprint functions is considered deprecated. Try to
//          avoid them whenever possible.

/** FORMATTED BUFFER PRINTING
 **
 **  bufprint() allows your to easily and safely append formatted string
 **  content to a given bounded character buffer, in a way that is easier
 **  to use than raw snprintf()
 **
 **  'buffer'  is the start position in the buffer,
 **  'buffend' is the end of the buffer, the function assumes (buffer <= buffend)
 **  'format'  is a standard printf-style format string, followed by any number
 **            of formatting arguments
 **
 **  the function returns the next position in the buffer if everything fits
 **  in it. in case of overflow or formatting error, it will always return "buffend"
 **
 **  this allows you to chain several calls to bufprint() and only check for
 **  overflow at the end, for exemple:
 **
 **     char   buffer[1024];
 **     char*  p   = buffer;
 **     char*  end = p + sizeof(buffer);
 **
 **     p = bufprint(p, end, "%s/%s", first, second);
 **     p = bufprint(p, end, "/%s", third);
 **     if (p >= end) ---> overflow
 **
 **  as a convenience, the appended string is zero-terminated if there is no overflow.
 **  (this means that even if p >= end, the content of "buffer" is zero-terminated)
 **
 **  vbufprint() is a variant that accepts a va_list argument
 **/

extern char*   vbufprint(char*  buffer, char*  buffend, const char*  fmt, va_list  args );
extern char*   bufprint (char*  buffer, char*  buffend, const char*  fmt, ... );

// Append the path to all emulator temporary files to a bounded |buffer| that
// stops at |buffend|, and return the new position.
extern char*  bufprint_temp_dir   (char*  buffer, char*  buffend);

// Append the path of a file or directory named |suffix| relative to the
// output of bufprint_temp_dir() to a bounded |buffer| that stops at
// |buffend|, and return the new position.
extern char*  bufprint_temp_file  (char*  buffer, char*  buffend, const char*  suffix);

ANDROID_END_HEADER
