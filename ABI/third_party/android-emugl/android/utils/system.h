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

#pragma once

#include <string.h>
#include <stdint.h>
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS 1
#endif
#include <inttypes.h>  /* for PRId64 et al. */
#include "android/utils/assert.h"
#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

/* internal helpers */
void*  _android_array_alloc( size_t  itemSize, size_t  count );
void*  _android_array_alloc0( size_t  itemSize, size_t  count );
void*  _android_array_realloc( void* block, size_t  itemSize, size_t  count );

/* the following functions perform 'checked allocations', i.e.
 * they abort if there is not enough memory.
 */

/* checked malloc, only returns NULL if size is 0 */
void*  android_alloc( size_t  size );

/* checked calloc, only returns NULL if size is 0 */
void*  android_alloc0( size_t  size );

/* checked realloc, only returns NULL if size if 0 */
void*  android_realloc( void*  block, size_t  size );

/* free memory block */
void   android_free( void*  block );

/* convenience macros */

#define  AZERO(p)             memset((char*)(p),0,sizeof(*(p)))
#ifdef __cplusplus
#define  ANEW(p)              (p = reinterpret_cast<__typeof__(*p)>(android_alloc(sizeof(*p))))
#define  ANEW0(p)             (p = reinterpret_cast<__typeof__(*p)>(android_alloc(sizeof(*p))))
#else  // !__cplusplus
#define  ANEW(p)              (p = android_alloc(sizeof(*p)))
#define  ANEW0(p)             (p = android_alloc0(sizeof(*p)))
#endif  // !__cplusplus
#define  AFREE(p)             android_free(p)

#define  AMEM_ZERO(dst,size)      memset((char*)(dst), 0, (size_t)(size))
#define  AMEM_COPY(dst,src,size)  memcpy((char*)(dst),(const char*)(src),(size_t)(size))
#define  AMEM_MOVE(dst,src,size)  memmove((char*)(dst),(const char*)(src),(size_t)(size))

#define  AARRAY_NEW(p,count)          (AASSERT_LOC(), (p) = _android_array_alloc(sizeof(*p),(count)))
#define  AARRAY_NEW0(p,count)         (AASSERT_LOC(), (p) = _android_array_alloc0(sizeof(*p),(count)))
#define  AARRAY_RENEW(p,count)        (AASSERT_LOC(), (p) = _android_array_realloc((p),sizeof(*(p)),(count)))

#define  AARRAY_COPY(dst,src,count)   AMEM_COPY(dst,src,(count)*sizeof((dst)[0]))
#define  AARRAY_MOVE(dst,src,count)   AMEM_MOVE(dst,src,(count)*sizeof((dst)[0]))
#define  AARRAY_ZERO(dst,count)       AMEM_ZERO(dst,(count)*sizeof((dst)[0]))

#define  AARRAY_STATIC_LEN(a)         (sizeof((a))/sizeof((a)[0]))

#define  AINLINED  static __inline__

/* unlike strdup(), this accepts NULL as valid input (and will return NULL then) */
char*   android_strdup(const char*  src);

#define  ASTRDUP(str)  android_strdup(str)

/* used for functions that return a Posix-style status code, i.e.
 * 0 means success, -1 means failure with the error code in 'errno'
 */
typedef int  APosixStatus;

/* used for functions that return or accept a boolean type */
typedef int  ABool;

/** Stringification macro
 **/
#ifndef STRINGIFY
#define  _STRINGIFY(x)  #x
#define  STRINGIFY(x)  _STRINGIFY(x)
#endif

/** Concatenation macros
 **/
#ifndef GLUE
#define  _GLUE(x,y)  x##y
#define  GLUE(x,y)   _GLUE(x,y)

#define  _GLUE3(x,y,z)  x##y##z
#define  GLUE3(x,y,z)    _GLUE3(x,y,z)
#endif

/** Handle strsep() on Win32
 **/
#ifdef _WIN32
#  undef   strsep
#  define  strsep    win32_strsep
extern char*  win32_strsep(char**  pline, const char*  delim);
#endif

/** Handle strcasecmp on Windows (and older Mingw32 toolchain)
 **/
#if defined(_WIN32) && !ANDROID_GCC_PREREQ(4,4)
#  define  strcasecmp  stricmp
#endif

/** SIGNAL HANDLING
 **
 ** the following can be used to block SIGALRM for a given period of time.
 ** use with caution, the QEMU execution loop uses SIGALRM extensively
 **
 **/
#ifdef _WIN32
typedef struct { int  dumy; }      signal_state_t;
#else
#include <signal.h>
typedef struct { sigset_t  old; }  signal_state_t;
#endif

extern  void   disable_sigalrm( signal_state_t  *state );
extern  void   restore_sigalrm( signal_state_t  *state );

#ifdef _WIN32

#define   BEGIN_NOSIGALRM  \
    {

#define   END_NOSIGALRM  \
    }

#else /* !WIN32 */

#define   BEGIN_NOSIGALRM  \
    { signal_state_t  __sigalrm_state; \
      disable_sigalrm( &__sigalrm_state );

#define   END_NOSIGALRM  \
      restore_sigalrm( &__sigalrm_state );  \
    }

#endif /* !WIN32 */

/** TIME HANDLING
 **
 ** sleep for a given time in milliseconds. note: this uses
 ** disable_sigalrm()/restore_sigalrm()
 **/

extern  void   sleep_ms( int  timeout );

/** FORMATTING int64_t in printf() statements
 **
 ** Normally defined in <inttypes.h> except on Windows and maybe others.
 **/

#ifndef PRId64
#  define PRId64  "lld"
#endif
#ifndef PRIx64
#  define PRIx64  "llx"
#endif
#ifndef PRIu64
#  define PRIu64  "llu"
#endif

// Getting thread id's (cross platform)


#ifdef _WIN32

#define ANDROID_THREADID_FMT "lx"
// cannot include windows yet, here.
// Should be equiv. to DWORD
#define android_thread_id_t long unsigned int

#elif defined(__APPLE__)

#define ANDROID_THREADID_FMT "x"
#define android_thread_id_t int

#elif defined(__linux__)

#define ANDROID_THREADID_FMT "lx"
#define android_thread_id_t long

#else

#error "Operating system not supported"
#endif

extern android_thread_id_t android_get_thread_id();

/** ****************************************************************************
 ** Various system functions exposed from the modern
 ** android/base/system/System.h implementation.
 **/

/** Get system / user time for the current process, in milliseconds **/
/* Type duplicated from android/base/system/System.h */
extern int64_t get_user_time_ms();
extern int64_t get_system_time_ms();

// Caller must free the returned string.
extern char* get_launcher_directory();
extern char* get_host_os_type();
extern void add_library_search_dir(const char* dirPath);

ANDROID_END_HEADER
